#ifndef TMULTIPLEXINGSERVER_H
#define TMULTIPLEXINGSERVER_H

#include <QThread>
#include <QMap>
#include <QByteArray>
#include <QFileInfo>
#include <QAtomicPointer>
#include <TGlobal>
#include <TApplicationServerBase>
#include <TAccessLog>

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
    void incomingRequest(int fd, const THttpRequest &request);

protected slots:
    void terminate();
    void deleteActionContext();

private:
    int maxWorkers;
    volatile bool stopped;
    int listenSocket;
    int epollFd;
    QMap<int, THttpBuffer> recvBuffers;
    QMap<int, THttpSendBuffer *> sendBuffers;

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
    QAtomicPointer<SendData> sendRequest;

    TMultiplexingServer();
    Q_DISABLE_COPY(TMultiplexingServer)
};

#endif // TMULTIPLEXINGSERVER_H
