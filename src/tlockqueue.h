#pragma once
#include <queue>
#include <mutex>
#include <optional>


template <typename T>
class TLockQueue {
public:
    void push(T val)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _queue.push(std::move(val));
    }

    std::optional<T> pop()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        std::optional<T> val;

        if (!_queue.empty()) {
            val = std::move(_queue.front());
            _queue.pop();
        }
        return val;
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _queue.empty();
    }

    size_t size() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _queue.size();
    }

    size_t count() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _queue.size();
    }

private:
    std::queue<T> _queue;
    mutable std::mutex _mutex;
};
