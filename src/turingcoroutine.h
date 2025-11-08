#pragma once
#include <TActionContext>
#include <QFile>
#include <coroutine>

class TUringCoroutine;
struct Task;

/*
struct Task {
    struct promise_type {
        TUringCoroutine *self {nullptr};
        Task get_return_object() { return {}; }
        std::suspend_never initial_suspend() noexcept { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() { std::terminate(); }
    };
};
*/

class TUringCoroutine : public TActionContext {
public:
    TUringCoroutine(int socketDescriptor) :
        TActionContext(), _sd(socketDescriptor) {}
    virtual ~TUringCoroutine();

    Task start();
    //static TUringCoroutine *currentRoutine();

protected:
    virtual int64_t writeResponse(THttpResponseHeader &, QIODevice *) override;

private:
    int _sd {0};
    QByteArray _response;
    QString _fileName;
};
