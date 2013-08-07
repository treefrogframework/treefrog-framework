#ifndef TMULTIPLEXINGSERVER_H
#define TMULTIPLEXINGSERVER_H

#include <QThread>
#include <QMap>
#include <QQueue>
#include <QList>
#include <QByteArray>
#include <QFileInfo>
#include <QAtomicPointer>
#include <TGlobal>
#include <TApplicationServerBase>
#include <TAccessLog>
#include "tatomicqueue.h"

class QIODevice;
class THttpRequest;
class THttpHeader;
class THttpBuffer;
class THttpSendBuffer;


class T_CORE_EXPORT TMultiplexingServer : public QThread, public TApplicationServerBase
{
    Q_OBJECT
public:
    ~TMultiplexingServer();

    bool isListening() const { return listenSocket > 0; }
    bool start();
    void stop() { stopped = true; }

    void setSendRequest(int fd, const THttpHeader *header, QIODevice *body, bool autoRemove, const TAccessLogger &accessLogger);
    void setDisconnectRequest(int fd);

    static void instantiate();
    static TMultiplexingServer *instance();

protected:
    void run();
    int epollAdd(int fd, int events);
    int epollModify(int fd, int events);
    int epollDel(int fd);
    void epollClose(int fd);
    int getSendRequest();
    void emitIncomingRequest(int fd, THttpBuffer &buffer);

signals:
    bool incomingHttpRequest(int fd, const QByteArray &request, const QString &address);

protected slots:
    void terminate();
    void deleteActionContext();

private:
    struct SendData
    {
        enum Method {
            Disconnect,
            Send,
        };
        int method;
        int fd;
        THttpSendBuffer *buffer;
    };

    int maxWorkers;
    volatile bool stopped;
    int listenSocket;
    int epollFd;
    QMap<int, THttpBuffer> recvBuffers;
    QMap<int, QQueue<THttpSendBuffer*> > sendBuffers;
    TAtomicQueue<SendData*> sendRequests;
    QList<int> pendingRequests;
    QAtomicInt threadCounter;

    TMultiplexingServer(QObject *parent = 0);  // Constructor
    friend class TWorkerStarter;
    Q_DISABLE_COPY(TMultiplexingServer)
};


/*
 * WorkerStarter class declaration
 * This object creates worker threads in the main event loop.
 */
class TWorkerStarter : public QObject
{
    Q_OBJECT
public:
    TWorkerStarter(QObject *parent = 0) : QObject(parent) { }
    virtual ~TWorkerStarter();

public slots:
    void startWorker(int fd, const QByteArray &request, const QString &address);
};

#endif // TMULTIPLEXINGSERVER_H
