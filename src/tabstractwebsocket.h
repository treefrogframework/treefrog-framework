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

    void sendText(const QString &message);
    void sendBinary(const QByteArray &data);
    void sendPing();
    void sendPong();
    void sendClose(int code);
    virtual void disconnect() = 0;

    static bool searchEndpoint(const THttpRequestHeader &header);
    static THttpResponseHeader handshakeResponse(const THttpRequestHeader &header);

protected:
    virtual qint64 writeRawData(const QByteArray &data) = 0;
    virtual QList<TWebSocketFrame> &websocketFrames() = 0;
    int parse(QByteArray &recvData);

    volatile bool closing;
    volatile bool closeSent;

    friend class TWebSocketWorker;
    Q_DISABLE_COPY(TAbstractWebSocket)
};

#endif // TABSTRACTWEBSOCKET_H
