#ifndef TWEBSOCKETCONTROLLER_H
#define TWEBSOCKETCONTROLLER_H

#include <QObject>
#include <QVariant>
#include <TGlobal>
#include <TSession>


class T_CORE_EXPORT TWebSocketController : public QObject
{
    Q_OBJECT
public:
    TWebSocketController() { }
    virtual ~TWebSocketController() { }

    virtual void onOpen(const TSession &session);
    virtual void onClose();
    virtual void onTextReceived(const QString &text);
    virtual void onBinaryReceived(const QByteArray &binary);
    virtual void onPing();
    virtual void onPong();

    QString className() const;
    QString name() const;

protected:
    void sendText(const QString &text);
    void sendBinary(const QByteArray &binary);
    void sendPing();
    void sendPong();
    void closeWebSocket();

private:
    mutable QString ctrlName;
    QVariantList payloadList;

    friend class TWsActionWorker;
    Q_DISABLE_COPY(TWebSocketController)
};



inline QString TWebSocketController::className() const
{
    return QString(metaObject()->className());
}

#endif // TWEBSOCKETCONTROLLER_H
