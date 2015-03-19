#ifndef APPLICATIONENDPOINT_H
#define APPLICATIONENDPOINT_H

#include "TWebSocketEndpoint"


class T_CONTROLLER_EXPORT ApplicationEndpoint : public TWebSocketEndpoint
{
    Q_OBJECT
public:
    ApplicationEndpoint();
    ApplicationEndpoint(const ApplicationEndpoint &other);

    void onOpen(const TSession &session);
    void onClose();
    void onTextReceived(const QString &text);
    void onBinaryReceived(const QByteArray &binary);
    void onPing();
    void onPong();
};

T_DECLARE_CONTROLLER(ApplicationEndpoint, applicationendpoint)

#endif // APPLICATIONENDPOINT_H
