#include "applicationendpoint.h"


ApplicationEndpoint::ApplicationEndpoint()
    : TWebSocketEndpoint()
{ }

ApplicationEndpoint::ApplicationEndpoint(const ApplicationEndpoint &)
    : TWebSocketEndpoint()
{ }

void ApplicationEndpoint::onOpen(const TSession &)
{ }

void ApplicationEndpoint::onClose(int)
{ }

void ApplicationEndpoint::onTextReceived(const QString &)
{ }

void ApplicationEndpoint::onBinaryReceived(const QByteArray &)
{ }

void ApplicationEndpoint::onPing()
{ }

void ApplicationEndpoint::onPong()
{ }

bool ApplicationEndpoint::transactionEnabled() const
{
    return TWebSocketEndpoint::transactionEnabled();
}
