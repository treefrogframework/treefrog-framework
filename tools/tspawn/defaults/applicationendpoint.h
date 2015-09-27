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
    virtual bool onOpen(const TSession &httpSession);
    virtual void onClose(int closeCode);
    virtual void onTextReceived(const QString &text);
    virtual void onBinaryReceived(const QByteArray &binary);
    virtual void onPing(const QByteArray &data);
    virtual void onPong(const QByteArray &data);
    virtual int keepAliveInterval() const;
    virtual bool transactionEnabled() const;
};

#endif // APPLICATIONENDPOINT_H
