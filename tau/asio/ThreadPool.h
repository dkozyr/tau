#pragma once

#include "tau/asio/Common.h"
#include <atomic>
#include <cstddef>

class ThreadPool {
public:
    ThreadPool(size_t threads_count);
    ~ThreadPool();

    void Join();

    Executor GetExecutor();
    Executor GetStrand();

private:
    asio::thread_pool _io;
    std::atomic_bool _is_joined = {false};
};
