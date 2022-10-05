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
    qint64 receiveData(char *buffer, qint64 length);
    QByteArray receiveAll();
    bool waitForDataSent(int msecs = 5000);
    bool waitForDataReceived(int msecs = 5000);
    bool isDataSent() const { return _sendBuffer.isEmpty(); }
    bool isDataReceived() const { return !_recvBuffer.isEmpty(); }
    void disconnect();
    void switchToWebSocket(const THttpRequestHeader &header);
    int bufferedListCount() const;
    bool autoDelete() const { return _autoDelete; }
    void setAutoDelete(bool autoDelete) { _autoDelete = autoDelete; }

    virtual bool canReadRequest() { return false; }
    virtual void process() { }

    static TSendBuffer *createSendBuffer(const QByteArray &header, const QFileInfo &file, bool autoRemove, const TAccessLogger &logger);
    static TSendBuffer *createSendBuffer(const QByteArray &data);

protected:
    virtual int send();
    virtual int recv();
    void enqueueSendData(TSendBuffer *buffer);
    void setSocketDescpriter(int socketDescriptor);
    virtual void *getRecvBuffer(int size);
    virtual bool seekRecvBuffer(int pos);
    static QSet<TEpollSocket *> allSockets();

    TAtomic<bool> pollIn {false};
    TAtomic<bool> pollOut {false};

    QByteArray _recvBuffer;  // Recieve-buffer

private:
    int _sd {0};  // socket descriptor
    QHostAddress _clientAddr;
    QQueue<TSendBuffer *> _sendBuffer;
    bool _autoDelete {true};

    static void initBuffer(int socketDescriptor);

    friend class TEpoll;
    friend class TMultiplexingServer;
    T_DISABLE_COPY(TEpollSocket)
    T_DISABLE_MOVE(TEpollSocket)
};
