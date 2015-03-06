/* Copyright (c) 2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TWebSocketController>
#include "twebsocketframe.h"


void TWebSocketController::onOpen(const TSession &session)
{
    Q_UNUSED(session);
}


void TWebSocketController::onClose()
{ }


void TWebSocketController::onTextReceived(const QString &text)
{
    Q_UNUSED(text);
}


void TWebSocketController::onBinaryReceived(const QByteArray &binary)
{
    Q_UNUSED(binary);
}


void TWebSocketController::onPing()
{ }


void TWebSocketController::onPong()
{ }


QString TWebSocketController::name() const
{
    if (ctrlName.isEmpty()) {
        ctrlName = className().remove(QRegExp("Controller$"));
    }
    return ctrlName;
}


void TWebSocketController::sendText(const QString &text)
{
    payloadList << QVariant(text);
}


void TWebSocketController::sendBinary(const QByteArray &binary)
{
    payloadList << QVariant(binary);
}


void TWebSocketController::sendPing()
{
    payloadList << QVariant((int)TWebSocketFrame::Ping);
}


void TWebSocketController::sendPong()
{
    payloadList << QVariant((int)TWebSocketFrame::Pong);
}


void TWebSocketController::closeWebSocket()
{
    payloadList << QVariant((int)TWebSocketFrame::Close);
}
