#pragma once
#include "turingserver.h"
#include <QThreadPool>
#include <coroutine>
#include <functional>
#include <optional>
#include <exception>
#include "tfcore.h"


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
            // 最大何個まで？ TODO TODO TODO TODO TODO TODO TODO TODO TODO
            QThreadPool::globalInstance()->setMaxThreadCount(128);
        });
    }

    ~TThreadPoolAwaiter() = default;

    bool await_ready() const noexcept { return false; }

    bool await_suspend(std::coroutine_handle<TUringTask::promise_type> handle)
    {
        _handle = handle;
        QThreadPool::globalInstance()->start([this, handle] {
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
