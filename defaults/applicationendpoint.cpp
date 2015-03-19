#include "applicationendpoint.h"


ApplicationEndpoint::ApplicationEndpoint()
    : TWebSocketEndpoint()
{ }

ApplicationEndpoint::ApplicationEndpoint(const ApplicationEndpoint &)
    : TWebSocketEndpoint()
{ }

void ApplicationEndpoint::onOpen(const TSession &)
{ }

void ApplicationEndpoint::onClose()
{ }

void ApplicationEndpoint::onTextReceived(const QString &)
{ }

void ApplicationEndpoint::onBinaryReceived(const QByteArray &)
{ }

void ApplicationEndpoint::onPing()
{ }

void ApplicationEndpoint::onPong()
{ }

// Don't remove below this line
T_REGISTER_CONTROLLER(applicationendpoint)
