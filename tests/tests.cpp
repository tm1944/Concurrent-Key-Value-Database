#include "command_parser.h"
#include "kv_store.h"

#include <functional>
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

}  // namespace

int main() {
    const std::vector<std::pair<std::string, std::function<void()>>> tests = {
        {"set and get", test_set_and_get},
        {"set overwrites", test_set_overwrites_existing_value},
        {"missing key", test_missing_key_returns_no_value},
        {"remove existing key", test_remove_existing_key},
        {"remove missing key", test_remove_missing_key},
        {"parse valid commands", test_parse_valid_commands},
        {"reject invalid commands", test_parser_rejects_invalid_commands},
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
