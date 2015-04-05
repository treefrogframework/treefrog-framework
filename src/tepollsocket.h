#ifndef TEPOLLSOCKET_H
#define TEPOLLSOCKET_H

#include <QObject>
#include <QByteArray>
#include <QQueue>
#include <QHostAddress>
#include <QAtomicInt>
#include <TGlobal>

class QHostAddress;
class QThread;
class QFileInfo;
class TSendBuffer;
class THttpHeader;
class TAccessLogger;
class THttpRequestHeader;


class T_CORE_EXPORT TEpollSocket : public QObject
{
    Q_OBJECT
public:
    TEpollSocket(int socketDescriptor, const QHostAddress &address);
    virtual ~TEpollSocket();

    void close();
    int socketDescriptor() const { return sd; }
    const QHostAddress &peerAddress() const { return clientAddr; }
    const QByteArray &socketUuid() const { return uuid; }
    void sendData(const QByteArray &header, QIODevice *body, bool autoRemove, const TAccessLogger &accessLogger);
    void sendData(const QByteArray &data);
    void disconnect();
    void switchToWebSocket(const THttpRequestHeader &header);

    virtual bool canReadRequest() { return false; }
    virtual void startWorker() { }

    static TEpollSocket *accept(int listeningSocket);
    static TEpollSocket *create(int socketDescriptor, const QHostAddress &address);
    static TSendBuffer *createSendBuffer(const QByteArray &header, const QFileInfo &file, bool autoRemove, const TAccessLogger &logger);
    static TSendBuffer *createSendBuffer(const QByteArray &data);

public slots:
    void deleteLater();

protected:
    int send();
    int recv();
    void enqueueSendData(TSendBuffer *buffer);
    void setSocketDescpriter(int socketDescriptor);
    int countWorkers() const;
    virtual void *getRecvBuffer(int size) = 0;
    virtual bool seekRecvBuffer(int pos) = 0;

    volatile bool deleting;
    QAtomicInt myWorkerCounter;

private:
    int sd;
    QByteArray uuid;
    QHostAddress clientAddr;
    QQueue<TSendBuffer*> sendBuf;

    static void initBuffer(int socketDescriptor);

    friend class TEpoll;
    Q_DISABLE_COPY(TEpollSocket)
};


inline int TEpollSocket::countWorkers() const
{
#if QT_VERSION >= 0x050000
    return myWorkerCounter.load();
#else
    return (int)myWorkerCounter;
#endif
}

#endif // TEPOLLSOCKET_H
