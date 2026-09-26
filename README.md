# Redis KV database

Redis KV database is a Redis-like key-value database written in C++17. The
current server accepts a TCP connection on port `6379`, reads newline-delimited
commands, and sends one newline-delimited response for each valid command.

The implemented commands are:

- `SET <key> <value>` stores a value and returns `OK`.
- `GET <key>` returns the value or `(nil)` when the key does not exist.
- `DELETE <key>` removes a key and returns `1`, or returns `0` when the key does
  not exist.
- `EXIT` is accepted by the parser and returns an empty response line.

The server currently handles one client per run. The in-memory store lasts for
that connection. Concurrent clients, `EXPIRE`, `TTL`, write-ahead logging,
crash recovery, and benchmarks are planned but are not implemented in this
repository yet.

## Run the server

Build the project, then start the server:

```sh
./build/server
```

In another terminal, connect with Netcat:

```sh
nc 127.0.0.1 6379
```

Each command must end with a newline. For example:

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

## Build and test

You need a C++17 compiler, CMake 3.16 or newer, and a POSIX system such as
Linux, macOS, or WSL. The TCP server uses POSIX socket headers.

```sh
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build --build-config Debug --output-on-failure
```

To run the test executable directly on a single-configuration generator such
as Make or Ninja:

```sh
./build/redis_kv_tests
```

With Visual Studio, the executable is usually under the selected configuration:

```powershell
.\build\Debug\redis_kv_tests.exe
```

## Add a test

Unit tests live in `tests/tests.cpp`. The TCP test lives in
`tests/tcp_server_tests.cpp`. Both files use small dependency-free runners.
Add a test function that throws through `require` when an expectation fails,
then register it in the file's `main` function.

```cpp
void test_example() {
    KVStore store;
    store.set("key", "value");

    require(store.get("key") == std::optional<std::string>{"value"},
            "GET should return the stored value");
}
```
