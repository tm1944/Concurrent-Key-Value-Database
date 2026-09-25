# Redis KV database

Redis KV database is a Redis-like key-value database written in C++17. It is
being built around a custom TCP server and command parser, with support planned
for `GET`, `SET`, `DELETE`, `EXPIRE`, and `TTL` commands.

The current repository contains the in-memory key-value store and command
parser. The broader design includes concurrent request handling through a
thread pool, mutex-protected shared state, and RAII ownership for sockets and
other system resources. Write-ahead logging will persist mutating operations,
and startup recovery will rebuild state by replaying the log. Concurrent client
benchmarks will measure throughput and request latency.

## Build and test

You need a C++17 compiler and CMake 3.16 or newer.

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

Tests live in `tests/tests.cpp` and use a small dependency-free runner. Add a
test function that throws through `require` when an expectation fails, then add
the function to the `tests` list in `main`.

```cpp
void test_example() {
    KVStore store;
    store.set("key", "value");

    require(store.get("key") == std::optional<std::string>{"value"},
            "GET should return the stored value");
}
```
