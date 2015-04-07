#ifndef APPLICATIONENDPOINT_H
#define APPLICATIONENDPOINT_H

#include "TWebSocketEndpoint"


class T_CONTROLLER_EXPORT ApplicationEndpoint : public TWebSocketEndpoint
{
public:
    ApplicationEndpoint();
    ApplicationEndpoint(const ApplicationEndpoint &other);
    virtual ~ApplicationEndpoint() { }

protected:
    virtual void onOpen(const TSession &session);
    virtual void onClose();
    virtual void onTextReceived(const QString &text);
    virtual void onBinaryReceived(const QByteArray &binary);
    virtual void onPing();
    virtual void onPong();
    virtual bool transactionEnabled() const;
};

#endif // APPLICATIONENDPOINT_H
