#pragma once
#include <TActionContext>
#include <QFile>
#include <coroutine>


struct Task {
    struct promise_type {
        Task get_return_object() { return {}; }
        std::suspend_never initial_suspend() noexcept { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() { std::terminate(); }
    };
};


class TUringCoroutine : public TActionContext {
public:
    TUringCoroutine() : TActionContext() {}
    virtual ~TUringCoroutine() {}

    Task start(int socketDescriptor);
    // static TActionRoutine *currentProcess();
    // static bool isChildProcess();
    //QByteArray response() const { return _response; }
    //static TActionContext *currentContext();

protected:
    virtual int64_t writeResponse(THttpResponseHeader &, QIODevice *) override;

private:
    QByteArray _response;
    QFile _file;
};
