#ifndef TWEBSOCKETCONTROLLER_H
#define TWEBSOCKETCONTROLLER_H

#include <QObject>
#include <TGlobal>
#include <TSession>


class T_CORE_EXPORT TWebSocketController : public QObject
{
    Q_OBJECT
public:
    TWebSocketController() { }
    virtual ~TWebSocketController() { }

    virtual void onOpen(const TSession &session) { }
    virtual void onClose() { }
    virtual void onTextReceived(const QString &text) { }
    virtual void onBinaryReceived(const QByteArray &binary) { }
    virtual void onPing() { }
    virtual void onPong() { }
};

#endif // TWEBSOCKETCONTROLLER_H
