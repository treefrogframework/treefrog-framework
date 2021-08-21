#pragma once
#include "tbasictimer.h"
#include <TAtomic>
#include <QByteArray>
#include <QList>
#include <QMutex>
#include <THttpRequestHeader>
#include <TWebSocketSession>
#include <TGlobal>

class QObject;
class THttpResponseHeader;
class TWebSocketFrame;


class T_CORE_EXPORT TAbstractWebSocket {
public:
    TAbstractWebSocket(const THttpRequestHeader &header);
    virtual ~TAbstractWebSocket();

    void sendText(const QString &message);
    void sendBinary(const QByteArray &data);
    void sendPing(const QByteArray &data = QByteArray());
    void sendPong(const QByteArray &data = QByteArray());
    void sendClose(int code);
    virtual void disconnect() = 0;
    virtual qintptr socketDescriptor() const = 0;
    virtual int socketId() const = 0;
    void startKeepAlive(int interval);
    void stopKeepAlive();
    void renewKeepAlive();
    TWebSocketSession session() const;
    void setSession(const TWebSocketSession &session);
    static bool searchEndpoint(const THttpRequestHeader &header);
    static TAbstractWebSocket *searchWebSocket(int sid);

protected:
    void sendHandshakeResponse();
    virtual QObject *thisObject() = 0;
    virtual qint64 writeRawData(const QByteArray &data) = 0;
    virtual QList<TWebSocketFrame> &websocketFrames() = 0;
    int parse(QByteArray &recvData);

    THttpRequestHeader reqHeader;
    TAtomic<bool> closing {false};
    TAtomic<bool> closeSent {false};
    mutable QMutex mutexData;
    TWebSocketSession sessionStore;
    TBasicTimer *keepAliveTimer {nullptr};

    friend class TWebSocketWorker;
    T_DISABLE_COPY(TAbstractWebSocket)
    T_DISABLE_MOVE(TAbstractWebSocket)
};

