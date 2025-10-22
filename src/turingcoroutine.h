#pragma once
#include <coroutine>
class TActionContext;


struct Task {
    struct promise_type {
        Task get_return_object() { return {}; }
        std::suspend_never initial_suspend() noexcept { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() { std::terminate(); }
    };
};


class TURingCoroutine {
public:
    static Task incomingConnection(int socketDescriptor);
    static TActionContext *currentContext();
};
