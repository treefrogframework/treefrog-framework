#ifndef TEPOLLWEBSOCKET_H
#define TEPOLLWEBSOCKET_H

#include <TGlobal>
#include <THttpRequestHeader>
#include <THttpResponseHeader>
#include "tepollsocket.h"

class QHostAddress;
class TWebSocketFrame;
class TSession;


class T_CORE_EXPORT TEpollWebSocket : public TEpollSocket
{
    Q_OBJECT
public:
    ~TEpollWebSocket();

    bool isTextRequest() const;
    bool isBinaryRequest() const;
    QString readTextRequest();
    QByteArray readBinaryRequest();
    virtual bool canReadRequest();
    virtual void startWorker();
    void startWorkerForOpening(const TSession &session);

    static bool validateHandshakeRequest(const THttpRequestHeader &header);
    static void sendText(const QByteArray &socketUuid, const QString &message);
    static void sendBinary(const QByteArray &socketUuid, const QByteArray &data);
    static void sendPing(const QByteArray &socketUuid);
    static void sendPong(const QByteArray &socketUuid);
    static void disconnect(const QByteArray &socketUuid);

protected:
    virtual void *getRecvBuffer(int size);
    virtual bool seekRecvBuffer(int pos);
    int parse();
    void clear();
    THttpResponseHeader handshakeResponse() const;

private:
    THttpRequestHeader reqHeader;
    QByteArray recvBuffer;
    QList<TWebSocketFrame> frames;

    TEpollWebSocket(int socketDescriptor, const QHostAddress &address, const THttpRequestHeader &header);

    friend class TEpoll;
    Q_DISABLE_COPY(TEpollWebSocket)
};

#endif // TEPOLLWEBSOCKET_H
