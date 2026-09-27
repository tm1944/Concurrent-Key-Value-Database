#include "command_executor.h"
#include "command_parser.h"
#include "kv_store.h"
#include "thread_pool.h"
#include "temp_log.h"
#include <set>
#include <sstream>

#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void test_set_and_get() {
    KVStore store;

    store.set("language", "C++");

    const auto value = store.get("language");
    require(value.has_value(), "GET should find a key stored by SET");
    require(*value == "C++", "GET should return the stored value");
}

void test_set_overwrites_existing_value() {
    KVStore store;
    store.set("status", "old");

    store.set("status", "new");

    require(store.get("status") == std::optional<std::string>{"new"},
            "SET should overwrite an existing value");
}

void test_missing_key_returns_no_value() {
    KVStore store;

    require(!store.get("missing").has_value(),
            "GET should return no value for a missing key");
}

void test_remove_existing_key() {
    KVStore store;
    store.set("temporary", "value");

    require(store.remove("temporary"),
            "DELETE should report that an existing key was removed");
    require(!store.get("temporary").has_value(),
            "DELETE should make the key unavailable");
}

void test_remove_missing_key() {
    KVStore store;

    require(!store.remove("missing"),
            "DELETE should report false for a missing key");
}

void test_executor_set_stores_value() {
    KVStore store;
    TempLog file;
    WriteAheadLog log(file.path());
    CommandExecutor executor(store, log);
    const Command command{Command::Type::SET, {"account", "active"}};

    const auto response = executor.execute(command);

    require(response == "OK", "SET should return OK");
    require(store.get("account") == std::optional<std::string>{"active"},
            "SET should store the supplied value");
}

void test_executor_get_existing_key() {
    KVStore store;
    store.set("account", "active");
    TempLog file;
    WriteAheadLog log(file.path());
    CommandExecutor executor(store, log);
    const Command command{Command::Type::GET, {"account"}};

    require(executor.execute(command) == "active",
            "GET should return the stored value");
}

void test_executor_get_missing_key() {
    KVStore store;
    TempLog file;
    WriteAheadLog log(file.path());
    CommandExecutor executor(store, log);
    const Command command{Command::Type::GET, {"missing"}};

    require(executor.execute(command) == "(nil)",
            "GET should return (nil) for a missing key");
}

void test_executor_delete_existing_key() {
    KVStore store;
    store.set("temporary", "value");
    TempLog file;
    WriteAheadLog log(file.path());
    CommandExecutor executor(store, log);
    const Command command{Command::Type::DELETE, {"temporary"}};

    require(executor.execute(command) == "1",
            "DELETE should return 1 when it removes a key");
    require(!store.get("temporary").has_value(),
            "DELETE should remove the key from the store");
}

void test_executor_delete_missing_key() {
    KVStore store;
    TempLog file;
    WriteAheadLog log(file.path());
    CommandExecutor executor(store, log);
    const Command command{Command::Type::DELETE, {"missing"}};

    require(executor.execute(command) == "0",
            "DELETE should return 0 when the key does not exist");
}

void test_parse_valid_commands() {
    CommandParser parser;

    const auto get = parser.parse("GET account");
    require(get.has_value(), "GET with one key should parse");
    require(get->type == Command::Type::GET, "GET should have the GET type");
    require(get->args == std::vector<std::string>{"account"},
            "GET should contain its key argument");

    const auto set = parser.parse("SET account active");
    require(set.has_value(), "SET with a key and value should parse");
    require(set->type == Command::Type::SET, "SET should have the SET type");
    require(set->args == std::vector<std::string>({"account", "active"}),
            "SET should contain its key and value arguments");

    const auto remove = parser.parse("DELETE account");
    require(remove.has_value(), "DELETE with one key should parse");
    require(remove->type == Command::Type::DELETE,
            "DELETE should have the DELETE type");

    const auto exit = parser.parse("EXIT");
    require(exit.has_value(), "EXIT without arguments should parse");
    require(exit->type == Command::Type::EXIT, "EXIT should have the EXIT type");
}

void test_parser_rejects_invalid_commands() {
    CommandParser parser;

    require(!parser.parse("").has_value(), "An empty command should be rejected");
    require(!parser.parse("UNKNOWN key").has_value(),
            "An unknown command should be rejected");
    require(!parser.parse("GET").has_value(),
            "GET without a key should be rejected");
    require(!parser.parse("SET only-a-key").has_value(),
            "SET without a value should be rejected");
    require(!parser.parse("DELETE key extra").has_value(),
            "DELETE with extra arguments should be rejected");
    require(!parser.parse("EXIT now").has_value(),
            "EXIT with arguments should be rejected");
}

void test_thread_pool_executes_enqueued_task() {
    std::promise<int> result;
    auto completed = result.get_future();
    ThreadPool pool(1);

    pool.enqueue([&result] { result.set_value(42); });

    require(completed.wait_for(std::chrono::seconds(1)) ==
                std::future_status::ready,
            "A worker should execute an enqueued task");
    require(completed.get() == 42,
            "The enqueued task should produce its expected result");
}

void test_thread_pool_destructor_finishes_queued_tasks() {
    std::atomic<int> completed{0};
    {
        ThreadPool pool(2);
        for (int i = 0; i < 20; ++i) {
            pool.enqueue([&completed] { ++completed; });
        }
    }

    require(completed.load() == 20,
            "Destroying the pool should finish every queued task");
}

void test_wal_records_and_reopen() {
    TempLog file;
    {
        WriteAheadLog log(file.path());
        log.set_log("account", "active");
        log.delete_log("account");
        require(file.read() == "SET account active\nDELETE account\n",
                "Records must be complete and visible after each write");
    }
    {
        WriteAheadLog log(file.path());
        log.set_log("next", "value");
    }
    require(file.read() == "SET account active\nDELETE account\nSET next value\n",
            "Reopening must append without truncating existing records");
}

void test_wal_rejects_invalid_path() {
    TempLog file;
    bool threw = false;
    try {
        WriteAheadLog log(file.path() + "/missing/log");
    } catch (const std::runtime_error&) {
        threw = true;
    }
    require(threw, "Opening a log below a nonexistent directory must fail");
}

void test_executor_logs_only_mutations() {
    TempLog file;
    WriteAheadLog log(file.path());
    KVStore store;
    CommandExecutor executor(store, log);
    require(executor.execute({Command::Type::SET, {"key", "first"}}) == "OK",
            "SET response");
    require(executor.execute({Command::Type::SET, {"key", "second"}}) == "OK",
            "Overwrite response");
    require(executor.execute({Command::Type::GET, {"key"}}) == "second",
            "GET must see the overwrite");
    require(executor.execute({Command::Type::DELETE, {"key"}}) == "1",
            "DELETE response");
    require(executor.execute({Command::Type::DELETE, {"key"}}) == "0",
            "Missing DELETE response");
    require(executor.execute({Command::Type::EXIT, {}}).empty(), "EXIT response");
    require(file.read() == "SET key first\nSET key second\nDELETE key\nDELETE key\n",
            "Only mutation commands must be logged, in execution order");
}

void test_concurrent_wal_records() {
    TempLog file;
    WriteAheadLog log(file.path());
    {
        ThreadPool pool(4);
        for (int i = 0; i < 200; ++i) {
            pool.enqueue([&log, i] { log.set_log(std::to_string(i), "value"); });
        }
    }
    std::istringstream input(file.read());
    std::multiset<std::string> actual;
    for (std::string line; std::getline(input, line);) actual.insert(line);
    std::multiset<std::string> expected;
    for (int i = 0; i < 200; ++i)
        expected.insert("SET " + std::to_string(i) + " value");
    require(actual == expected, "Every concurrent record must occur exactly once");
}

void test_parser_whitespace_and_case() {
    CommandParser parser;
    const auto command = parser.parse(" \tSET\tkey value\r");
    require(command && command->type == Command::Type::SET &&
                command->args == std::vector<std::string>{"key", "value"},
            "Whitespace should separate tokens");
    for (const auto* invalid : {"set key value", "GET key extra",
                               "SET key value extra", "DELETE", " \t"}) {
        require(!parser.parse(invalid), "Invalid syntax must be rejected");
    }
}

}  // namespace

int main() {
    const std::vector<std::pair<std::string, std::function<void()>>> tests = {
        {"set and get", test_set_and_get},
        {"WAL records and reopen", test_wal_records_and_reopen},
        {"WAL invalid path", test_wal_rejects_invalid_path},
        {"executor WAL integration", test_executor_logs_only_mutations},
        {"concurrent WAL records", test_concurrent_wal_records},
        {"parser whitespace and case", test_parser_whitespace_and_case},
        {"set overwrites", test_set_overwrites_existing_value},
        {"missing key", test_missing_key_returns_no_value},
        {"remove existing key", test_remove_existing_key},
        {"remove missing key", test_remove_missing_key},
        {"executor SET", test_executor_set_stores_value},
        {"executor GET existing", test_executor_get_existing_key},
        {"executor GET missing", test_executor_get_missing_key},
        {"executor DELETE existing", test_executor_delete_existing_key},
        {"executor DELETE missing", test_executor_delete_missing_key},
        {"parse valid commands", test_parse_valid_commands},
        {"reject invalid commands", test_parser_rejects_invalid_commands},
        {"thread pool executes task", test_thread_pool_executes_enqueued_task},
        {"thread pool drains tasks", test_thread_pool_destructor_finishes_queued_tasks},
    };

    int failures = 0;
    for (const auto& [name, test] : tests) {
        try {
            test();
            std::cout << "[PASS] " << name << '\n';
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "[FAIL] " << name << ": " << error.what() << '\n';
        }
    }

    std::cout << '\n' << tests.size() - failures << '/' << tests.size()
              << " tests passed\n";
    return failures == 0 ? 0 : 1;
}
