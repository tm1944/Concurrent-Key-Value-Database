#pragma once

#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <cstdlib>
#include <vector>

class TempLog {
public:
    TempLog() {
        auto pattern = (std::filesystem::temp_directory_path() /
                        "redis-kv-tests-XXXXXX").string();
        std::vector<char> buffer(pattern.begin(), pattern.end());
        buffer.push_back('\0');
        const auto directory = mkdtemp(buffer.data());
        if (!directory) throw std::runtime_error("Cannot create test directory");
        directory_ = directory;
    }
    ~TempLog() {
        std::error_code error;
        std::filesystem::remove_all(directory_, error);
    }
    TempLog(const TempLog&) = delete;
    TempLog& operator=(const TempLog&) = delete;
    std::string path() const { return (directory_ / "test.wal").string(); }
    std::string read() const {
        std::ifstream input(path());
        if (!input) throw std::runtime_error("Cannot read test log");
        return {std::istreambuf_iterator<char>(input),
                std::istreambuf_iterator<char>()};
    }
private:
    std::filesystem::path directory_;
};
