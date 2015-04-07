/* Copyright (c) 2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TWebSocketEndpoint>
#include <TActionController>
#include "twebsocketframe.h"


TWebSocketEndpoint::TWebSocketEndpoint()
    : ctrlName(), payloadList(), rollback(false)
{ }

void TWebSocketEndpoint::onOpen(const TSession &session)
{
    Q_UNUSED(session);
}


void TWebSocketEndpoint::onClose()
{ }


void TWebSocketEndpoint::onTextReceived(const QString &text)
{
    Q_UNUSED(text);
}


void TWebSocketEndpoint::onBinaryReceived(const QByteArray &binary)
{
    Q_UNUSED(binary);
}


void TWebSocketEndpoint::onPing()
{ }


void TWebSocketEndpoint::onPong()
{ }


QString TWebSocketEndpoint::name() const
{
    if (ctrlName.isEmpty()) {
        ctrlName = className().remove(QRegExp("Controller$"));
    }
    return ctrlName;
}


void TWebSocketEndpoint::sendText(const QString &text)
{
    payloadList << QVariant(text);
}


void TWebSocketEndpoint::sendBinary(const QByteArray &binary)
{
    payloadList << QVariant(binary);
}


void TWebSocketEndpoint::sendPing()
{
    payloadList << QVariant((int)TWebSocketFrame::Ping);
}


void TWebSocketEndpoint::sendPong()
{
    payloadList << QVariant((int)TWebSocketFrame::Pong);
}


void TWebSocketEndpoint::closeWebSocket()
{
    payloadList << QVariant((int)TWebSocketFrame::Close);
}


bool TWebSocketEndpoint::isUserLoggedIn(const TSession &session)
{
    return session.contains(TActionController::loginUserNameKey());
}


QString TWebSocketEndpoint::identityKeyOfLoginUser(const TSession &session)
{
    return session.value(TActionController::loginUserNameKey()).toString();
}


const QStringList &TWebSocketEndpoint::disabledEndpoints()
{
    static const QStringList disabledNames = { "application" };
    return disabledNames;
}
