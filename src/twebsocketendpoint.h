#ifndef TWEBSOCKETENDPOINT_H
#define TWEBSOCKETENDPOINT_H

#include <QObject>
#include <QStringList>
#include <QPair>
#include <QVariant>
#include <TGlobal>
#include <TSession>
#include <TWebSocketSession>


class T_CORE_EXPORT TWebSocketEndpoint : public QObject
{
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
    void sendText(const QByteArray &uuid, const QString &text);
    void sendBinary(const QByteArray &uuid, const QByteArray &binary);
    void close(const QByteArray &uuid, int closeCode = Tf::NormalClosure);
    void rollbackTransaction();
    void subscribe(const QString &topic, bool local = true);
    void unsubscribe(const QString &topic);
    void unsubscribeFromAll();
    void publish(const QString &topic, const QString &text);
    void publish(const QString &topic, const QByteArray &binary);
    void startKeepAlive(int interval);
    void sendHttp(const QByteArray &uuid, const QByteArray &data);
    const TWebSocketSession &session() const { return sessionStore; }
    TWebSocketSession &session() { return sessionStore; }
    QByteArray socketUuid() const { return uuid; }

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
    QByteArray uuid;
    QList<QPair<int, QVariant>> taskList;
    bool rollback;

    friend class TWebSocketWorker;
    Q_DISABLE_COPY(TWebSocketEndpoint)
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

#endif // TWEBSOCKETENDPOINT_H
