#ifndef TABSTRACTWEBSOCKET_H
#define TABSTRACTWEBSOCKET_H

#include <QList>
#include <QByteArray>
#include <QMutex>
#include <TGlobal>
#include <TWebSocketSession>
#include <THttpRequestHeader>
#include <atomic>
#include "tbasictimer.h"

class QObject;
class THttpResponseHeader;
class TWebSocketFrame;


class T_CORE_EXPORT TAbstractWebSocket
{
public:
    TAbstractWebSocket(const THttpRequestHeader &header);
    virtual ~TAbstractWebSocket();

    void sendText(const QString &message);
    void sendBinary(const QByteArray &data);
    void sendPing(const QByteArray &data = QByteArray());
    void sendPong(const QByteArray &data = QByteArray());
    void sendClose(int code);
    virtual void disconnect() = 0;
    virtual QByteArray socketUuid() const = 0;
    void startKeepAlive(int interval);
    void stopKeepAlive();
    void renewKeepAlive();
    TWebSocketSession session() const;
    void setSession(const TWebSocketSession &session);
    static bool searchEndpoint(const THttpRequestHeader &header);
    static TAbstractWebSocket *searchWebSocket(const QByteArray &uuid);

protected:
    void sendHandshakeResponse();
    virtual QObject *thisObject() = 0;
    virtual qint64 writeRawData(const QByteArray &data) = 0;
    virtual QList<TWebSocketFrame> &websocketFrames() = 0;
    int parse(QByteArray &recvData);

    THttpRequestHeader reqHeader;
    std::atomic<bool> closing;
    std::atomic<bool> closeSent;
    mutable QMutex mutexData;
    TWebSocketSession sessionStore;
    TBasicTimer *keepAliveTimer;

    friend class TWebSocketWorker;
    Q_DISABLE_COPY(TAbstractWebSocket)
};

#endif // TABSTRACTWEBSOCKET_H
