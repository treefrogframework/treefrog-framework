#pragma once
#include <stack>
#include <mutex>
#include <optional>


template <typename T>
class TLockStack {
public:
    TLockStack() = default;
    TLockStack(const TLockStack &) = delete;
    TLockStack &operator=(const TLockStack &) = delete;
    TLockStack(TLockStack &&other) noexcept
    {
        _stack = std::move(other._stack);
    }
    TLockStack &operator=(TLockStack &&) = delete;

    void push(const T &val)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _stack.push(val);
    }

    void push(T &&val)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _stack.push(std::move(val));
    }

    std::optional<T> pop()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        std::optional<T> val;

        if (!_stack.empty()) {
            val = std::forward<T>(_stack.top());
            _stack.pop();
        }
        return val;
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _stack.empty();
    }

    size_t size() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _stack.size();
    }

    size_t count() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _stack.size();
    }

private:
    std::stack<T> _stack;
    mutable std::mutex _mutex;
};
