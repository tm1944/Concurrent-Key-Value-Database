#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <thread>
#include <queue>
#include <mutex>
#include <vector>


class ThreadPool {
public:
    explicit ThreadPool(std::size_t num_threads);

    void enqueue(std::function<void()> task);

    ~ThreadPool();
private:
    void worker_loop_(); //function continiously exevuted by each worker thread

    std::vector<std::thread> workers_; //"threads owned by the pool"
    std::queue<std::function<void()>> tasks_; //taks waiting to be executed

    std::mutex mutex_;

    std::condition_variable  condition_;

    bool stopping_ = false; //tells workers pool is shutting down

};
