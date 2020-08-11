#pragma once
#include <QHostAddress>
#include <QObject>
#include <QPair>
#include <QStringList>
#include <QVariant>
#include <TGlobal>
#include <TSession>
#include <TWebSocketSession>


class T_CORE_EXPORT TWebSocketEndpoint : public QObject {
public:
    TWebSocketEndpoint();
    virtual ~TWebSocketEndpoint() { }

    QString className() const;
    QString name() const;
    void sendText(const QString &text);
    void sendBinary(const QByteArray &binary);
    void ping(const QByteArray &payload = QByteArray());
    void sendPing(const QByteArray &payload = QByteArray());
    void close(int closeCode = Tf::NormalClosure);
    void sendText(int sid, const QString &text);
    void sendBinary(int sid, const QByteArray &binary);
    void closeSocket(int sid, int closeCode = Tf::NormalClosure);
    void rollbackTransaction();
    void subscribe(const QString &topic, bool local = true);
    void unsubscribe(const QString &topic);
    void unsubscribeFromAll();
    void publish(const QString &topic, const QString &text);
    void publish(const QString &topic, const QByteArray &binary);
    void startKeepAlive(int interval);
    void sendHttp(int sid, const QByteArray &data);
    const TWebSocketSession &session() const { return sessionStore; }
    TWebSocketSession &session() { return sessionStore; }
    int socketId() const { return sid; }
    QHostAddress peerAddress() const { return peerAddr; }
    quint16 peerPort() const { return peerPortNumber; }

    static bool isUserLoggedIn(const TSession &session);
    static QString identityKeyOfLoginUser(const TSession &session);
    static const QStringList &disabledEndpoints();

protected:
    virtual bool onOpen(const TSession &session);
    virtual void onClose(int closeCode);
    virtual void onTextReceived(const QString &text);
    virtual void onBinaryReceived(const QByteArray &binary);
    virtual void onPing(const QByteArray &payload);
    virtual void onPong(const QByteArray &payload);
    virtual int keepAliveInterval() const { return 0; }
    virtual bool transactionEnabled() const;
    void sendPong(const QByteArray &payload = QByteArray());

private:
    enum TaskType {
        OpenSuccess = 0,
        OpenError,
        SendText,
        SendBinary,
        SendClose,
        SendPing,
        SendPong,
        SendTextTo,
        SendBinaryTo,
        SendCloseTo,
        Subscribe,
        Unsubscribe,
        UnsubscribeFromAll,
        PublishText,
        PublishBinary,
        StartKeepAlive,
        StopKeepAlive,
        HttpSend,
    };

    bool rollbackRequested() const;

    TWebSocketSession sessionStore;
    int sid {0};
    QList<QPair<int, QVariant>> taskList;
    bool rollback {false};
    QHostAddress peerAddr;
    quint16 peerPortNumber {0};

    friend class TWebSocketWorker;
    T_DISABLE_COPY(TWebSocketEndpoint)
    T_DISABLE_MOVE(TWebSocketEndpoint)
};


inline QString TWebSocketEndpoint::className() const
{
    return QString(metaObject()->className());
}

inline void TWebSocketEndpoint::rollbackTransaction()
{
    rollback = true;
}

inline bool TWebSocketEndpoint::rollbackRequested() const
{
    return rollback;
}

inline bool TWebSocketEndpoint::transactionEnabled() const
{
    return false;
}

