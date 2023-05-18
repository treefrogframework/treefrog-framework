#pragma once
#include "tatomic.h"
#include <TGlobal>
#include <QByteArray>
#include <QHostAddress>
#include <QQueue>

class TSendBuffer;
class THttpHeader;
class TAccessLogger;
class THttpRequestHeader;
class QHostAddress;
class QThread;
class QFileInfo;


class T_CORE_EXPORT TEpollSocket {
public:
    TEpollSocket();
    TEpollSocket(int socketDescriptor, Tf::SocketState state, const QHostAddress &peerAddress);
    virtual ~TEpollSocket();

    void connectToHost(const QHostAddress &address, uint16_t port);
    void close();
    void dispose();
    int socketDescriptor() const { return _socket; }
    QHostAddress peerAddress() const { return _peerAddress; }
    void sendData(const QByteArray &header, QIODevice *body, bool autoRemove, TAccessLogger &&accessLogger);
    void sendData(const QByteArray &data);
    int64_t receivedSize() const { return _recvBuffer.length(); }
    int64_t receiveData(char *buffer, int64_t length);
    QByteArray receiveAll();
    bool waitForConnected(int msecs = 5000);
    bool waitForDataSent(int msecs = 5000);
    bool waitForDataReceived(int msecs = 5000);
    bool waitUntil(bool (TEpollSocket::*method)(), int msecs = 5000);
    bool isConnected() const { return state() == Tf::SocketState::Connected; }
    bool isDataSent() const { return _sendBuffer.isEmpty(); }
    bool isDataReceived() const { return !_recvBuffer.isEmpty(); }
    void disconnect();
    void switchToWebSocket(const THttpRequestHeader &header);
    int bufferedListCount() const;
    bool autoDelete() const { return _autoDelete; }
    void setAutoDelete(bool autoDelete) { _autoDelete = autoDelete; }
    Tf::SocketState state() const { return _state; }
    void setSocketDescriptor(int socketDescriptor);
    bool setSocketOption(int level, int optname, int val);
    bool watch();

    virtual bool canReadRequest() { return false; }
    virtual void process() { }
    virtual bool isProcessing() const { return false; }

    static TSendBuffer *createSendBuffer(const QByteArray &header, const QFileInfo &file, bool autoRemove, TAccessLogger &&logger);
    static TSendBuffer *createSendBuffer(const QByteArray &data);

protected:
    virtual int send();
    virtual int recv();
    void enqueueSendData(TSendBuffer *buffer);
    virtual void *getRecvBuffer(int size);
    virtual bool seekRecvBuffer(int pos);
    static QSet<TEpollSocket *> allSockets();

    QByteArray _recvBuffer;  // Recieve-buffer

private:
    int _socket {0};  // socket descriptor
    Tf::SocketState _state {Tf::SocketState::Unconnected};
    QHostAddress _peerAddress;
    QQueue<TSendBuffer *> _sendBuffer;
    bool _autoDelete {true};

    static void initBuffer(int socketDescriptor);

    friend class TEpoll;
    friend class TMultiplexingServer;
    T_DISABLE_COPY(TEpollSocket)
    T_DISABLE_MOVE(TEpollSocket)
};
