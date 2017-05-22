#include "applicationendpoint.h"


ApplicationEndpoint::ApplicationEndpoint()
    : TWebSocketEndpoint()
{ }

bool ApplicationEndpoint::onOpen(const TSession &)
{
    return true;
}

void ApplicationEndpoint::onClose(int)
{ }

void ApplicationEndpoint::onTextReceived(const QString &)
{ }

void ApplicationEndpoint::onBinaryReceived(const QByteArray &)
{ }

void ApplicationEndpoint::onPing(const QByteArray &data)
{
    TWebSocketEndpoint::onPing(data);
}

void ApplicationEndpoint::onPong(const QByteArray &)
{ }

int ApplicationEndpoint::keepAliveInterval() const
{
    return 0;
}

bool ApplicationEndpoint::transactionEnabled() const
{
    return TWebSocketEndpoint::transactionEnabled();
}
