#ifndef TEPOLLSOCKET_H
#define TEPOLLSOCKET_H

#include <QObject>
#include <QByteArray>
#include <QQueue>
#include <QHostAddress>
#include <TGlobal>
#include <atomic>

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
    QHostAddress peerAddress() const { return clientAddr; }
    QByteArray socketUuid() const { return uuid; }
    void sendData(const QByteArray &header, QIODevice *body, bool autoRemove, const TAccessLogger &accessLogger);
    void sendData(const QByteArray &data);
    void disconnect();
    void switchToWebSocket(const THttpRequestHeader &header);
    int countWorker() const { return myWorkerCounter; }
    qint64 bufferedBytes() const;
    int bufferedListCount() const;

    virtual bool canReadRequest() { return false; }
    virtual void startWorker() { }
    virtual void deleteLater();

    static TEpollSocket *accept(int listeningSocket);
    static TEpollSocket *create(int socketDescriptor, const QHostAddress &address);
    static TSendBuffer *createSendBuffer(const QByteArray &header, const QFileInfo &file, bool autoRemove, const TAccessLogger &logger);
    static TSendBuffer *createSendBuffer(const QByteArray &data);

protected:
    virtual int send();
    virtual int recv();
    void enqueueSendData(TSendBuffer *buffer);
    void setSocketDescpriter(int socketDescriptor);
    virtual void *getRecvBuffer(int size) = 0;
    virtual bool seekRecvBuffer(int pos) = 0;

    std::atomic<bool> deleting;
    std::atomic<int> myWorkerCounter;
    std::atomic<bool> pollIn;
    std::atomic<bool> pollOut;

private:
    int sd;
    QByteArray uuid;
    QHostAddress clientAddr;
    QQueue<TSendBuffer*> sendBuf;

    static void initBuffer(int socketDescriptor);

    friend class TEpoll;
    friend class TMultiplexingServer;
    Q_DISABLE_COPY(TEpollSocket)
};

#endif // TEPOLLSOCKET_H
