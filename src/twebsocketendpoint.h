#ifndef TWEBSOCKETENDPOINT_H
#define TWEBSOCKETENDPOINT_H

#include <QObject>
#include <QVariant>
#include <TGlobal>
#include <TSession>


class T_CORE_EXPORT TWebSocketEndpoint : public QObject
{
public:
    TWebSocketEndpoint() { }
    virtual ~TWebSocketEndpoint() { }

    virtual void onOpen(const TSession &session);
    virtual void onClose();
    virtual void onTextReceived(const QString &text);
    virtual void onBinaryReceived(const QByteArray &binary);
    virtual void onPing();
    virtual void onPong();

    QString className() const;
    QString name() const;

    static const QStringList &disabledEndpoints();

protected:
    void sendText(const QString &text);
    void sendBinary(const QByteArray &binary);
    void sendPing();
    void sendPong();
    void closeWebSocket();

private:
    mutable QString ctrlName;
    QVariantList payloadList;

    friend class TWebSocketWorker;
    Q_DISABLE_COPY(TWebSocketEndpoint)
};



inline QString TWebSocketEndpoint::className() const
{
    return QString(metaObject()->className());
}

#endif // TWEBSOCKETENDPOINT_H
