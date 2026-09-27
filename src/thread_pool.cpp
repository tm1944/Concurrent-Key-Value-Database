#include "../include/thread_pool.h"
#include <iostream>


ThreadPool::ThreadPool(std::size_t num_thread){
    if (num_thread  == 0) return;
    
    workers_.reserve(num_thread);
    for (std::size_t i = 0; i < num_thread; ++i){
        workers_.emplace_back(&ThreadPool::worker_loop_, this);
    }
}

ThreadPool::~ThreadPool(){
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
    }
    condition_.notify_all();
    for(auto& t : workers_){
        if(t.joinable()){
            t.join();
        }
    }
}

void ThreadPool::worker_loop_(){
    while(true){
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            condition_.wait(lock, [this] {
                return !tasks_.empty() || stopping_; 
            });
            
            if(stopping_ && tasks_.empty()){
                return;
            }
            task = std::move(tasks_.front());
            tasks_.pop();
        }
        task();
    }
}


void ThreadPool::enqueue(std::function<void()> task){
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_.push(std::move(task));
    }
    condition_.notify_one();
}