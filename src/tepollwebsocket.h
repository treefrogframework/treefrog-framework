#ifndef TEPOLLWEBSOCKET_H
#define TEPOLLWEBSOCKET_H

#include <QUuid>
#include <TGlobal>
#include <THttpRequestHeader>
#include <THttpResponseHeader>
#include "tepollsocket.h"

class QHostAddress;
class TWebSocketFrame;


class T_CORE_EXPORT TEpollWebSocket : public TEpollSocket
{
    Q_OBJECT
public:
    enum OpCode {
        Continuation = 0x0,
        TextFrame    = 0x1,
        BinaryFrame  = 0x2,
        Reserve3     = 0x3,
        Reserve4     = 0x4,
        Reserve5     = 0x5,
        Reserve6     = 0x6,
        Reserve7     = 0x7,
        Close        = 0x8,
        Ping         = 0x9,
        Pong         = 0xA,
        ReserveB     = 0xB,
        ReserveC     = 0xC,
        ReserveD     = 0xD,
        ReserveE     = 0xE,
        ReserveF     = 0xF,
    };

    ~TEpollWebSocket();

    QByteArray socketUuid() const { return uuid.toByteArray(); }
    void sendText(const QString &message);
    void sendBinary(const QByteArray &data);

    bool isTextRequest() const;
    bool isBinaryRequest() const;
    QString readTextRequest();
    QByteArray readBinaryRequest();
    virtual bool canReadRequest();
    virtual void startWorker();

    static void sendText(const QByteArray &socketUuid, const QString &message);
    static void sendBinary(const QByteArray &socketUuid, const QByteArray &data);

protected:
    virtual void *getRecvBuffer(int size);
    virtual bool seekRecvBuffer(int pos);
    int parse();
    void clear();
    THttpResponseHeader handshakeResponse() const;

private:
    QUuid uuid;
    THttpRequestHeader reqHeader;
    QByteArray recvBuffer;
    QList<TWebSocketFrame> frames;

    TEpollWebSocket(int socketDescriptor, const QHostAddress &address, const THttpRequestHeader &header);

    friend class TEpoll;
    Q_DISABLE_COPY(TEpollWebSocket)
};

#endif // TEPOLLWEBSOCKET_H
