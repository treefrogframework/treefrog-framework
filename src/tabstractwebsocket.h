#ifndef TABSTRACTWEBSOCKET_H
#define TABSTRACTWEBSOCKET_H

#include <QList>
#include <QByteArray>
#include <QMutex>
#include <TGlobal>
#include <atomic>

class QObject;
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
    void sendPing(const QByteArray &data = QByteArray());
    void sendPong(const QByteArray &data = QByteArray());
    void sendClose(int code);
    virtual void disconnect() = 0;
    void startKeepAlive(int interval);
    void stopKeepAlive();
    void renewKeepAlive();

    static bool searchEndpoint(const THttpRequestHeader &header);
    static THttpResponseHeader handshakeResponse(const THttpRequestHeader &header);

protected:
    virtual QObject *thisObject() = 0;
    virtual qint64 writeRawData(const QByteArray &data) = 0;
    virtual QList<TWebSocketFrame> &websocketFrames() = 0;
    int parse(QByteArray &recvData);

    std::atomic<bool> closing;
    std::atomic<bool> closeSent;
    QMutex mutexKeepAlive;
    int keepAliveTimerId;
    int keepAliveInterval;

    friend class TWebSocketWorker;
    Q_DISABLE_COPY(TAbstractWebSocket)
};

#endif // TABSTRACTWEBSOCKET_H
