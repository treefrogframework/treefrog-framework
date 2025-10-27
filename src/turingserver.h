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
#include <TGlobal>
#include <coroutine>
#include <liburing.h>

class QIODevice;
class THttpHeader;
class THttpSendBuffer;
class TEpollSocket;
class TActionWorker;
class TActionController;


class TAwaitBase {
public:
    virtual ~TAwaitBase() { }
    virtual bool await_ready() const noexcept { return false; }
    inline void clear()
    {
        _cqeres = 0;
        _sqecounter = 1;
    }

    std::coroutine_handle<> _handle{};
    int _cqeres{0};
    int _sqecounter{1};
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
    //int processEvents(int maxMilliSeconds);
    TActionContext *currentContext() const;
    TActionController *currentController() const;

    //static void instantiate(int listeningSocket);
    static TUringServer *instance(int listeningSocket = 0);

    //
    int addAccept(int fd, TAwaitBase* await = nullptr);
    int addRecv(int fd, void* buf, size_t len, int msecs = 0, TAwaitBase* await = nullptr);
    int addSend(int fd, const void* buf, size_t len, TAwaitBase* await = nullptr);
    int addEvent(int fd, TAwaitBase* await = nullptr);

protected:
    void run() override;

private:
    io_uring _ring{};
    bool _stopped {false};
    int _listenSocket {0};
    //QBasicTimer reloadTimer;
    bool _autoReload {false};
    //mutable QStack<TActionWorker *> _processingSocketStack;
    //mutable QSet<TEpollSocket *> _garbageSockets;
    friend class TEpollSocket;
    T_DISABLE_COPY(TUringServer)
    T_DISABLE_MOVE(TUringServer)
};


/*
 * WorkerStarter class declaration
 * This object creates worker threads in the main event loop.
 */
/*
class TWorkerStarter : public QObject
{
    Q_OBJECT
public:
    TWorkerStarter(QObject *parent = 0) : QObject(parent) { }
    virtual ~TWorkerStarter();

public slots:
    void startWorker(TEpollSocket *);
};
*/

