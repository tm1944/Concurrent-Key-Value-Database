# Redis KV database

Redis KV database is an in-memory key-value server written in C++17. It listens
on TCP port `6379` and uses a thread pool with four workers. All clients share
one store, and a mutex protects concurrent access.

The server accepts newline-delimited commands and returns one newline-delimited
response for each valid command.

## Supported commands

- `SET <key> <value>` stores a value and returns `OK`.
- `GET <key>` returns the value. It returns `(nil)` if the key does not exist.
- `DELETE <key>` removes a key. It returns `1` if the key existed and `0` if it
  did not exist.
- `EXIT` returns an empty response line. It does not close the connection.

Keys and values cannot contain spaces. An invalid command closes the client
connection.

The database stores data in memory only. Restarting the server clears all keys.
The project does not implement expiration, persistence, authentication, or the
Redis wire protocol.

## Requirements

You need the following software:

- A C++17 compiler
- CMake 3.16 or newer
- Linux, macOS, or WSL

The TCP server and its integration tests use POSIX sockets, so they do not build
natively on Windows.

## Build the project

Run these commands from the repository root:

```sh
cmake -S . -B build
cmake --build build
```

## Run the server

Start the server after the build completes:

```sh
./build/server
```

Open another terminal and connect with Netcat:

```sh
nc 127.0.0.1 6379
```

Enter one command per line:

```text
SET account active
GET account
DELETE account
GET account
```

The server responds with:

```text
OK
active
1
(nil)
```

Open more Netcat sessions to use the server from several clients at the same
time. Each session accesses the same in-memory store.

## Run the tests

Build and run all tests with CTest:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The test suite covers command parsing, command execution, store operations,
thread-pool shutdown, fragmented TCP messages, batched commands, and concurrent
clients.

Run either test executable directly when you need its full output:

```sh
./build/redis_kv_tests
./build/tcp_server_tests
```

## Add a test

Unit tests live in `tests/tests.cpp`. TCP integration tests live in
`tests/tcp_server_tests.cpp`. Each file has a small test runner with no external
test-framework dependency.

Create a test function that calls `require` for each expected result. Register
the function in the `tests` list in `main`:

```cpp
void test_example() {
    KVStore store;
    store.set("key", "value");

    require(store.get("key") == std::optional<std::string>{"value"},
            "GET should return the stored value");
}
```
