cmake_minimum_required(VERSION 3.16)

project(redis_kv_db LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(redis_kv_core
    src/command_executor.cpp
    src/command_parser.cpp
    src/kv_store.cpp
    src/tcp_server.cpp
)

target_include_directories(redis_kv_core PUBLIC include)

add_executable(server
    src/main.cpp
)

target_link_libraries(server PRIVATE redis_kv_core)

if(MSVC)
    target_compile_options(redis_kv_core PRIVATE /W4)
else()
    target_compile_options(redis_kv_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)

if(BUILD_TESTING)
    add_executable(redis_kv_tests tests/tests.cpp)
    target_link_libraries(redis_kv_tests PRIVATE redis_kv_core)

    add_test(NAME redis_kv_tests COMMAND redis_kv_tests)

    if(UNIX)
        find_package(Threads REQUIRED)

        add_executable(tcp_server_tests tests/tcp_server_tests.cpp)
        target_link_libraries(tcp_server_tests PRIVATE
            redis_kv_core
            Threads::Threads
        )

        add_test(NAME tcp_server_tests COMMAND tcp_server_tests)
    endif()
endif()
