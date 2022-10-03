#pragma once
#include "tatomic.h"
#include <QByteArray>
#include <QHostAddress>
#include <QObject>
#include <QQueue>
#include <TGlobal>

class TSendBuffer;
class THttpHeader;
class TAccessLogger;
class THttpRequestHeader;
class QHostAddress;
class QThread;
class QFileInfo;


class T_CORE_EXPORT TEpollSocket {
public:
    TEpollSocket(int socketDescriptor, const QHostAddress &address);
    virtual ~TEpollSocket();

    void close();
    int socketDescriptor() const { return _sd; }
    QHostAddress peerAddress() const { return _clientAddr; }
    void sendData(const QByteArray &header, QIODevice *body, bool autoRemove, const TAccessLogger &accessLogger);
    void sendData(const QByteArray &data);
    void disconnect();
    void switchToWebSocket(const THttpRequestHeader &header);
    int bufferedListCount() const;

    virtual bool canReadRequest() { return false; }
    virtual void startWorker() { }

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
    static QSet<TEpollSocket *> allSockets();

    TAtomic<bool> pollIn {false};
    TAtomic<bool> pollOut {false};

private:
    int _sd {0};  // socket descriptor
    //int _sid {0};
    QHostAddress _clientAddr;
    QQueue<TSendBuffer *> _sendBuf;

    static void initBuffer(int socketDescriptor);

    friend class TEpoll;
    friend class TMultiplexingServer;
    T_DISABLE_COPY(TEpollSocket)
    T_DISABLE_MOVE(TEpollSocket)
};
