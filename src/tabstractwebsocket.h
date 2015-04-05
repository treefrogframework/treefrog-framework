#ifndef TABSTRACTWEBSOCKET_H
#define TABSTRACTWEBSOCKET_H

#include <QList>
#include <QByteArray>
#include <TGlobal>

class THttpRequestHeader;
class THttpResponseHeader;
class TWebSocketFrame;


class T_CORE_EXPORT TAbstractWebSocket
{
public:
    virtual ~TAbstractWebSocket();

    virtual void sendText(const QString &message) = 0;
    virtual void sendBinary(const QByteArray &data) = 0;
    virtual void sendPing() = 0;
    virtual void sendPong() = 0;
    virtual void disconnect() = 0;

    static bool searchEndpoint(const THttpRequestHeader &header);
    static THttpResponseHeader handshakeResponse(const THttpRequestHeader &header);

protected:
    int parse(QByteArray &recvData);
    virtual QList<TWebSocketFrame> &websocketFrames() = 0;
};

#endif // TABSTRACTWEBSOCKET_H
