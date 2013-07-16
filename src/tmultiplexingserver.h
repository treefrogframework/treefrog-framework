#ifndef TMULTIPLEXINGRECEIVER_H
#define TMULTIPLEXINGRECEIVER_H

#include <QThread>
#include <QMap>
#include <QByteArray>
#include <QPair>
#include <QAtomicPointer>
#include <TGlobal>

class QIODevice;
class THttpRequest;
class THttpHeader;
class THttpBuffer;


class T_CORE_EXPORT TMultiplexingReceiver : public QThread
{
public:
    ~TMultiplexingReceiver();

    bool isListening() const { return listenSocket > 0; }
    void stop() { stopped = true; }
    qint64 setSendRequest(int fd, const QByteArray &buffer);
    qint64 setSendRequest(int fd, const THttpHeader *header, QIODevice *body);

    static void instantiate();
    static TMultiplexingReceiver *instance();

protected:
    void run();
    int epollAdd(int fd, int events);
    int epollModify(int fd, int events);
    void epollClose(int fd);
    void incomingRequest(int fd, const THttpRequest &request);

private:
    volatile bool stopped;
    int listenSocket;
    int epollFd;
    QMap<int, THttpBuffer> bufferings;
    QAtomicPointer<QPair<int, QByteArray> > sendRequest;

    TMultiplexingReceiver();
    Q_DISABLE_COPY(TMultiplexingReceiver)
};

#endif // TMULTIPLEXINGRECEIVER_H
