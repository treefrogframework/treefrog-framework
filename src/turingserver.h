#pragma once
#include "tatomic.h"
#include <QBasicTimer>
#include <QByteArray>
#include <QFileInfo>
#include <QList>
#include <QMap>
#include <QStack>
#include <TAccessLog>
#include <TApplicationServerBase>
#include <TDatabaseContextThread>
#include <TSystemGlobal>
#include <TGlobal>
#include <coroutine>
#include <liburing.h>

class QIODevice;
class THttpHeader;
class THttpSendBuffer;
class TEpollSocket;
class TActionWorker;
class TActionController;
class TUringCoroutine;


class Task {
public:
    class promise_type {
    public:
        TUringCoroutine *self {nullptr};
        Task get_return_object()
        {
            return Task{ std::coroutine_handle<promise_type>::from_promise(*this) };
        }
        std::suspend_never initial_suspend() noexcept { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() { std::terminate(); }
    };

    explicit Task(std::coroutine_handle<promise_type> h) : handle(h) {}
    std::coroutine_handle<promise_type> handle;
};


class TAwaitBase {
public:
    virtual ~TAwaitBase() { }
    virtual bool await_ready() const noexcept { return false; }
    inline void clear()
    {
        _cqeres = 0;
        _cqeflags = 0;
        _sqecounter = 1;
    }

    std::coroutine_handle<Task::promise_type> _handle{};
    int _cqeres {0};
    int _cqeflags {0};
    int _sqecounter {1};
};


class T_CORE_EXPORT TUringServer : public TDatabaseContextThread, public TApplicationServerBase {
    Q_OBJECT
public:
    TUringServer(int listeningSocket, QObject *parent = nullptr);  // Constructor
    ~TUringServer();

    bool isListening() const { return _listenSocket > 0; }
    bool start(bool debugMode) override;
    void stop() override;
    void setAutoReloadingEnabled(bool enable) override;
    bool isAutoReloadingEnabled() override;
    TActionContext *currentContext() const;
    TActionController *currentController() const;
    void registerForGC(TUringCoroutine *);
    static TUringServer *instance(int listeningSocket = 0);

    int addAccept(int sd, TAwaitBase *await = nullptr) const;
    int addRecv(int sd, void *buf, size_t len, int msecs = 0, TAwaitBase *await = nullptr) const;
    int addSend(int sd, const void* buf, size_t len, TAwaitBase *await = nullptr) const;
    int addSendZc(int sd, const void* buf, size_t len, TAwaitBase *await = nullptr) const;
    int addEvent(int sd, TAwaitBase *await = nullptr) const;

protected:
    void run() override;

private:
    mutable io_uring _ring {};
    bool _stopped {false};
    int _listenSocket {0};
    bool _autoReload {false};
    TUringCoroutine *_currentCoroutine {nullptr};
    std::list<TUringCoroutine*> _garbage;
    friend class TEpollSocket;

    T_DISABLE_COPY(TUringServer)
    T_DISABLE_MOVE(TUringServer)
};

