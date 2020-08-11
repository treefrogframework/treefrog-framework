#pragma once
#include "TWebSocketEndpoint"


class T_CONTROLLER_EXPORT ApplicationEndpoint : public TWebSocketEndpoint
{
public:
    ApplicationEndpoint();
    virtual ~ApplicationEndpoint() { }

protected:
    virtual bool onOpen(const TSession &httpSession) override;
    virtual void onClose(int closeCode) override;
    virtual void onTextReceived(const QString &text) override;
    virtual void onBinaryReceived(const QByteArray &binary) override;
    virtual void onPing(const QByteArray &data) override;
    virtual void onPong(const QByteArray &data) override;
    virtual int keepAliveInterval() const override;
    virtual bool transactionEnabled() const override;
};

