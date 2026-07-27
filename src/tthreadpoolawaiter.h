#pragma once
#include "turingserver.h"
#include "tfcore.h"
#include <coroutine>
#include <functional>
#include <optional>
#include <thread>
#include <vector>
#include <queue>
#include <exception>


class ThreadPool
{
public:
    ~ThreadPool()
    {
        _stopping = true;
        _queueTaskCount.release(_workers.size());
        _workers.clear();
    }

    void setMaxThreadCount(size_t count)
    {
        _maxCount = std::max(_maxCount, count);
        _workers.reserve(_maxCount);
    }

    static ThreadPool &globalInstance()
    {
        static ThreadPool pool;
        return pool;
    }

    void start(std::function<void()> func)
    {
        if (_stopping.load(std::memory_order_relaxed)) {
            return;
        }

        _capacityCount.acquire();  // Waits until the queue has free space

        std::lock_guard locker {_mutexTasks};  // Locks mutex

        _tasks.push(std::move(func));
        _queueTaskCount.release();  // Increments and notify when a task is added

        if (_tasks.size() > _idleCount && _workers.size() < _maxCount) {
            try {
                // Worker thread function
                _workers.emplace_back([this]() {
                    std::function<void()> task;

                    while (!_stopping.load(std::memory_order_relaxed)) {
                        _mutexTasks.lock();

                        if (_tasks.empty()) {
                            _idleCount++;
                            _mutexTasks.unlock();
                            _queueTaskCount.acquire();  // Waits for new task
                            _idleCount--;
                            continue;
                        } else {
                            task = std::move(_tasks.front());
                            _tasks.pop();
                            _capacityCount.release();  // Notify that space has become available in queue
                        }

                        _mutexTasks.unlock();

                        // Executes
                        task();
                    }
                });
            } catch (...) {
                tSystemError("Thread create error  [{}:{}]", __FILE__, __LINE__);
                Tf::fatal("Thread create error  [{}:{}]", __FILE__, __LINE__);
                return;
            }
        }
    }

private:
    ThreadPool() = default;
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;

private:
    size_t _maxCount {4};
    std::vector<std::jthread> _workers {4};
    std::queue<std::function<void()>> _tasks;
    std::mutex _mutexTasks;
    std::counting_semaphore<INT_MAX> _queueTaskCount {0};  // Queue task count
    std::counting_semaphore<256> _capacityCount {256};  // Queue capacity count (max:256)
    std::atomic<size_t> _idleCount {0};
    std::atomic<bool> _stopping {false};
};


template<typename Func>
class TThreadPoolAwaiter : public TAwaitBase {
public:
    using FuncType = std::decay_t<Func>;
    using ReturnType = std::invoke_result_t<FuncType&>;
    using Result = std::conditional_t<std::is_void_v<ReturnType>, std::monostate, ReturnType>;

    TThreadPoolAwaiter(Func &&f) :
        _func(std::forward<Func>(f))
    {
        static std::once_flag once;

        std::call_once(once, []() {
            int num = Tf::app()->maxNumberOfThreadsPerAppServer();
            ThreadPool::globalInstance().setMaxThreadCount(num);
        });
    }

    ~TThreadPoolAwaiter() = default;

    bool await_ready() const noexcept { return false; }

    bool await_suspend(std::coroutine_handle<TUringTask::promise_type> handle)
    {
        _handle = handle;
        ThreadPool::globalInstance().start([this, handle] {
            try {
                if constexpr (std::is_void_v<ReturnType>) {
                    _func();
                } else {
                    _result = _func();
                }
            } catch (...) {
                handle.promise().exptr = std::current_exception();
            }

            TUringServer::instance()->addResumeHandle(handle);
        });
        return true;
    }

    Result await_resume()
    {
        if (_handle.promise().exptr) {
            std::rethrow_exception(_handle.promise().exptr);
        }

        if constexpr (std::is_void_v<ReturnType>) {
            return std::monostate{};
        }
        return std::move(_result);
    }

private:
    FuncType _func;
    Result _result;
};
