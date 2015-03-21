#ifndef APPLICATIONENDPOINT_H
#define APPLICATIONENDPOINT_H

#include "TWebSocketEndpoint"


class T_CONTROLLER_EXPORT ApplicationEndpoint : public TWebSocketEndpoint
{
public:
    ApplicationEndpoint();
    ApplicationEndpoint(const ApplicationEndpoint &other);
    virtual ~ApplicationEndpoint() { }

    void onOpen(const TSession &session);
    void onClose();
    void onTextReceived(const QString &text);
    void onBinaryReceived(const QByteArray &binary);
    void onPing();
    void onPong();
};

#endif // APPLICATIONENDPOINT_H
