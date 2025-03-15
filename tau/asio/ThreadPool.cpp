#include "tau/asio/ThreadPool.h"

namespace tau {

ThreadPool::ThreadPool(size_t threads_count)
    : _io(threads_count)
{}

ThreadPool::~ThreadPool() {
    Join();
}

void ThreadPool::Join() {
    if(!_is_joined.exchange(true)) {
        _io.join();
    }
}

Executor ThreadPool::GetExecutor() {
    return _io.get_executor();
}

Executor ThreadPool::GetStrand() {
    return asio::strand<Executor>(_io.get_executor());
}

}
