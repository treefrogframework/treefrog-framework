/* Copyright (c) 2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TWebSocketEndpoint>
#include <TActionController>
#include "twebsocketframe.h"


/*!
  \class TWebSocketEndpoint
  \brief The TWebSocketEndpoint is the base class of endpoints for
  WebSocket communication.
 */

TWebSocketEndpoint::TWebSocketEndpoint()
    : taskList(), rollback(false)
{ }

void TWebSocketEndpoint::onOpen(const TSession &session)
{
    Q_UNUSED(session);
}


void TWebSocketEndpoint::onClose(int closeCode)
{
    Q_UNUSED(closeCode);
}


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
    return className().remove(QRegExp("Endpoint$"));
}


void TWebSocketEndpoint::sendText(const QString &text)
{
    taskList << qMakePair((int)SendText, QVariant(text));
}


void TWebSocketEndpoint::sendBinary(const QByteArray &binary)
{
    taskList << qMakePair((int)SendBinary, QVariant(binary));
}


void TWebSocketEndpoint::sendPing()
{
    taskList << qMakePair((int)SendPing, QVariant());
}


void TWebSocketEndpoint::sendPong()
{
    taskList << qMakePair((int)SendPong, QVariant());
}


void TWebSocketEndpoint::close(int closeCode)
{
    taskList << qMakePair((int)SendClose, QVariant(closeCode));
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


void TWebSocketEndpoint::subscribe(const QString &topic)
{
    taskList << qMakePair((int)Subscribe, QVariant(topic));
}


void TWebSocketEndpoint::unsubscribe(const QString &topic)
{
    taskList << qMakePair((int)Unsubscribe, QVariant(topic));
}


void TWebSocketEndpoint::unsubscribeFromAll()
{
    taskList << qMakePair((int)UnsubscribeFromAll, QVariant());
}


void TWebSocketEndpoint::publish(const QString &topic, const QString &text)
{
    QVariantList message;
    message << topic << text;
    taskList << qMakePair((int)PublishText, QVariant(message));
}


void TWebSocketEndpoint::publish(const QString &topic, const QByteArray &binary)
{
    QVariantList message;
    message << topic << binary;
    taskList << qMakePair((int)PublishBinary, QVariant(message));
}
