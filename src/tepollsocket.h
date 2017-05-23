#ifndef TEPOLLSOCKET_H
#define TEPOLLSOCKET_H

#include <QObject>
#include <QByteArray>
#include <QHostAddress>
#include <TGlobal>
#include "tatomic.h"
#include "tqueue.h"

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
    int socketId() const { return sid; }
    void sendData(const QByteArray &header, QIODevice *body, bool autoRemove, const TAccessLogger &accessLogger);
    void sendData(const QByteArray &data);
    void disconnect();
    void switchToWebSocket(const THttpRequestHeader &header);
    int countWorker() const { return myWorkerCounter; }
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
    static TEpollSocket *searchSocket(int sid);
    static QList<TEpollSocket*> allSockets();

    TAtomic<bool> deleting {false};
    TAtomic<int> myWorkerCounter {0};
    TAtomic<bool> pollIn {false};
    TAtomic<bool> pollOut {false};

private:
    int sd {0};  // socket descriptor
    int sid {0};
    QHostAddress clientAddr;
    TQueue<TSendBuffer*> sendBuf;

    static void initBuffer(int socketDescriptor);

    friend class TEpoll;
    friend class TMultiplexingServer;
    T_DISABLE_COPY(TEpollSocket)
    T_DISABLE_MOVE(TEpollSocket)
};

#endif // TEPOLLSOCKET_H
