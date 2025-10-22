#include "turingcoroutine.h"
#include "turingserver.h"
#include "tactionroutine.h"
#include "TSystemGlobal"
#include "TAppSettings"
#include "THttpRequest"
#include <QStack>
#include <memory>
#include <cstddef>
#include <sys/eventfd.h>


class AsyncRecv : public TAwaitBase {
public:
    AsyncRecv(int fd, void* buffer, size_t length, int msecs) :
        _fd(fd), _buf(buffer), _len(length), _msecs(msecs) { }

    void await_suspend(std::coroutine_handle<> handle)
    {
        //std::print("== AsyncRecv \n");
        _handle = handle;
        if (TURingServer::instance()->addRecv(_fd, _buf, _len, _msecs, this) < 0) {
            tSystemError("addRecv error: {}", strerror(errno));
        }
    }

    inline int await_resume() { return _cqeres; }

private:
    int _fd{0};
    void* _buf{nullptr};
    size_t _len{0};
    int _msecs{0};
};


class AsyncSend : public TAwaitBase {
public:
    AsyncSend(int fd, const void* buf, size_t len) :
        _fd(fd), _buf(buf), _len(len) { }

    void await_suspend(std::coroutine_handle<> handle)
    {
        //std::print("== AsyncSend \n");
        _handle = handle;
        if (TURingServer::instance()->addSend(_fd, _buf, _len, this) < 0) {
            tSystemError("addSend error: {}", strerror(errno));
        }
    }

    inline int await_resume() { return _cqeres; }

private:
    int _fd{0};
    const void* _buf{nullptr};
    size_t _len{0};
};


template <typename R>
class AsyncFunction : public TAwaitBase {
public:
    explicit AsyncFunction(std::function<R()> f) :
        TAwaitBase(), _func(std::move(f)) {}
    ~AsyncFunction() { if (_fd > 0) ::close(_fd); }

    void await_suspend(std::coroutine_handle<> handle)
    {
        _handle = handle;
        _fd = eventfd(0, (EFD_NONBLOCK | EFD_CLOEXEC));
        TURingServer::instance()->addEvent(_fd, this);
        std::thread([this]() {
            _result = _func();
            notifyResume();
        }).detach();
    }

    R await_resume() { return _result; }

protected:
    void notifyResume()
    {
        if (_fd > 0) {
            uint64_t one = 1;
            write(_fd, &one, sizeof(one)); // 通知
        }
    }

private:
    int _fd{0};
    std::function<R()> _func;
    R _result{};
};


// void AsyncRecv::await_suspend(std::coroutine_handle<> handle)
// {
//     //std::print("== AsyncRecv \n");
//     _handle = handle;
//     if (TURingServer::instance()->addRecv(_fd, _buf, _len, _msecs, this) < 0) {
//         tSystemError("addRecv error: {}", strerror(errno));
//     }
// }


// void AsyncSend::await_suspend(std::coroutine_handle<> handle)
// {
//     //std::print("== AsyncSend \n");
//     _handle = handle;
//     if (TURingServer::instance()->addSend(_fd, _buf, _len, this) < 0) {
//         tSystemError("addSend error: {}", strerror(errno));
//     }
// }


template<typename Func>
class ScopeExitFunction {
public:
    explicit ScopeExitFunction(Func&& func) : _func(std::move(func)) {}
    ~ScopeExitFunction() noexcept { _func(); }
private:
    Func _func;
};



QStack<TActionContext *> _processingContext;


Task TURingCoroutine::incomingConnection(int sd)
{
    static int keepAlivetimeout = Tf::appSettings()->value(Tf::HttpKeepAliveTimeout).toInt();

    //std::print("accept {}\n", sd);
    ScopeExitFunction closing([sd]{ if (sd > 0) ::close(sd); });
    int res;

    // ソケット受信
    //int tmp = ++counter;
    char buf[124];
    AsyncRecv receiver(sd, buf, sizeof(buf), 5000);
    res = co_await receiver;
    //std::print("[RECV] fd={} res={}\n", sd, len);
    if (res <= 0) {
        if (res < 0) {
            tSystemError("Recv error fd:{} error:{}\n", sd, strerror(-res));
        } else {
            tSystemError("Recv peer closed fd:{}\n", sd);
        }
        co_return;
    }

    std::string s(buf, buf + res);
    //std::print("[RECV] recv len:{} {}\n", res, s);
    //std::print("[Main] ({}) Received ({} bytes): {}\n", tmp, len, s);


    THttpRequest request;
    auto context = std::make_unique<TActionRoutine>();
    auto response = context->start(request);


    // std::string response = "HTTP/1.1 200 OK\r\n"
    //     "Content-Type: text/plain; charset=utf-8\r\n"
    //     "Content-Length: 13\r\n"
    //     "Connection: close\r\n"
    //     "\r\n"
    //     "Hello world.\n";


/*
    if (sock->canReadRequest()) {
        _processingSocketStack.push(sock);
        sock->process();
        _processingSocketStack.pop();
    }
*/

    std::string dummy;
    AsyncSend sender(sd, dummy.c_str(), dummy.length());
    res = co_await sender;
    if (res <= 0) {
        tSystemError("Send error fd={} res={}\n", sd, res);
        co_return;
    }
    //std::print("send ({}) len:{}\n", tmp, res);
    //std::print("[SEND] fd={} res={}\n", sd, res);
}


TActionContext *TURingCoroutine::currentContext()
{
    if (!_processingContext.isEmpty()) {
        return _processingContext.top();
    }
    return nullptr;
}
