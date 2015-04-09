#ifndef TWEBSOCKETENDPOINT_H
#define TWEBSOCKETENDPOINT_H

#include <QObject>
#include <QStringList>
#include <QVariant>
#include <TGlobal>
#include <TSession>


class T_CORE_EXPORT TWebSocketEndpoint : public QObject
{
public:
    TWebSocketEndpoint();
    virtual ~TWebSocketEndpoint() { }

    QString className() const;
    QString name() const;
    void sendText(const QString &text);
    void sendBinary(const QByteArray &binary);
    void sendPing();
    void sendPong();
    void close(int closeCode = Tf::NormalClosure);
    void rollbackTransaction();
    bool rollbackRequested() const;

    static bool isUserLoggedIn(const TSession &session);
    static QString identityKeyOfLoginUser(const TSession &session);
    static const QStringList &disabledEndpoints();

protected:
    virtual void onOpen(const TSession &session);
    virtual void onClose(int closeCode);
    virtual void onTextReceived(const QString &text);
    virtual void onBinaryReceived(const QByteArray &binary);
    virtual void onPing();
    virtual void onPong();
    virtual bool transactionEnabled() const;

private:
    QVariantList payloadList;
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
