#ifndef TMULTIPLEXINGSERVER_H
#define TMULTIPLEXINGSERVER_H

#include <QThread>
#include <QMap>
#include <QByteArray>
#include <QPair>
#include <QAtomicPointer>
#include <TGlobal>
#include <TApplicationServerBase>

class QIODevice;
class THttpRequest;
class THttpHeader;
class THttpBuffer;


class T_CORE_EXPORT TMultiplexingServer : public QThread, public TApplicationServerBase
{
    Q_OBJECT
public:
    ~TMultiplexingServer();

    bool isListening() const { return listenSocket > 0; }
    bool start();
    void stop() { stopped = true; }
    qint64 setSendRequest(int fd, const QByteArray &buffer);
    qint64 setSendRequest(int fd, const THttpHeader *header, QIODevice *body);

    static void instantiate();
    static TMultiplexingServer *instance();

protected:
    void run();
    int epollAdd(int fd, int events);
    int epollModify(int fd, int events);
    void epollClose(int fd);
    void incomingRequest(int fd, const THttpRequest &request);

protected slots:
    void terminate();
    void deleteActionContext();

private:
    int maxServers;
    volatile bool stopped;
    int listenSocket;
    int epollFd;
    QMap<int, THttpBuffer> bufferings;
    QAtomicPointer<QPair<int, QByteArray> > sendRequest;

    TMultiplexingServer();
    Q_DISABLE_COPY(TMultiplexingServer)
};

#endif // TMULTIPLEXINGSERVER_H
