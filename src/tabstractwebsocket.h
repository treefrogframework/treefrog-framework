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
    TAbstractWebSocket();
    virtual ~TAbstractWebSocket();

    static bool searchEndpoint(const THttpRequestHeader &header);
    static THttpResponseHeader handshakeResponse(const THttpRequestHeader &header);

protected:
    int parse(QByteArray &recvData);
    virtual QList<TWebSocketFrame> &websocketFrames() = 0;
};

#endif // TABSTRACTWEBSOCKET_H
