/* Copyright (c) 2015-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "twebsocketframe.h"
#include <TActionController>
#include <TWebSocketEndpoint>


/*!
  \class TWebSocketEndpoint
  \brief The TWebSocketEndpoint is the base class of endpoints for
  WebSocket communication.
 */

TWebSocketEndpoint::TWebSocketEndpoint() :
    taskList(), rollback(false)
{
}

/*!
  This handler is called immediately after the WebSocket connection is
  open. If it returns false then the connection will be closed.
*/
bool TWebSocketEndpoint::onOpen(const TSession &session)
{
    Q_UNUSED(session);
    return true;
}

/*!
  This handler is called immediately when the WebSocket connection is
  closed.
*/
void TWebSocketEndpoint::onClose(int closeCode)
{
    Q_UNUSED(closeCode);
}

/*!
  This handler is called immediately after a text message is received from
  the client.
*/
void TWebSocketEndpoint::onTextReceived(const QString &text)
{
    Q_UNUSED(text);
}

/*!
  This handler is called immediately after a binary message is received from
  the client.
*/
void TWebSocketEndpoint::onBinaryReceived(const QByteArray &binary)
{
    Q_UNUSED(binary);
}

/*!
  This handler is called immediately after a Ping frame is received from
  the client.
*/
void TWebSocketEndpoint::onPing(const QByteArray &payload)
{
    sendPong(payload);
}

/*!
  This handler is called immediately after a Pong frame is received from
  the client.
*/
void TWebSocketEndpoint::onPong(const QByteArray &payload)
{
    Q_UNUSED(payload);
}

/*!
  Returns the endpoint name.
*/
QString TWebSocketEndpoint::name() const
{
    return className().remove(QRegularExpression("Endpoint$"));
}

/*!
  Sends the given \a text over the socket as a text message.
*/
void TWebSocketEndpoint::sendText(const QString &text)
{
    taskList << qMakePair((int)SendText, QVariant(text));
}

/*!
  Sends the given \a binary over the socket as a binary message.
*/
void TWebSocketEndpoint::sendBinary(const QByteArray &binary)
{
    taskList << qMakePair((int)SendBinary, QVariant(binary));
}

/*!
  Pings the client to indicate that the connection is still alive.
*/
void TWebSocketEndpoint::sendPing(const QByteArray &payload)
{
    ping(payload);
}

/*!
  Pings the client to indicate that the connection is still alive.
*/
void TWebSocketEndpoint::ping(const QByteArray &payload)
{
    taskList << qMakePair((int)SendPing, QVariant(payload));
}

/*!
  Sends a pong message. Internal use.
*/
void TWebSocketEndpoint::sendPong(const QByteArray &payload)
{
    taskList << qMakePair((int)SendPong, QVariant(payload));
}

/*!
  Disconnects the WebSocket's connection with the client, closes
  the socket.
*/
void TWebSocketEndpoint::close(int closeCode)
{
    taskList << qMakePair((int)SendClose, QVariant(closeCode));
}

/*!
  Sends the given \a text over the socket of the \a id as a text message.
*/
void TWebSocketEndpoint::sendText(int sid, const QString &text)
{
    QVariantList info;
    info << sid << text;
    taskList << qMakePair((int)SendTextTo, QVariant(info));
}

/*!
  Sends the given \a binary over the socket of the \a id as a binary message.
*/
void TWebSocketEndpoint::sendBinary(int sid, const QByteArray &binary)
{
    QVariantList info;
    info << sid << binary;
    taskList << qMakePair((int)SendBinaryTo, QVariant(info));
}

/*!
  Disconnects the WebSocket's connection of the \a id, closes
  the socket.
*/
void TWebSocketEndpoint::closeSocket(int sid, int closeCode)
{
    QVariantList info;
    info << sid << closeCode;
    taskList << qMakePair((int)SendCloseTo, QVariant(info));
}

/*!
  Returns true if a user is logged in to the system; otherwise returns
  false.
*/
bool TWebSocketEndpoint::isUserLoggedIn(const TSession &session)
{
    return session.contains(TActionController::loginUserNameKey());
}

/*!
  Returns the identity key of the user, TAbstractUser object, logged in.
*/
QString TWebSocketEndpoint::identityKeyOfLoginUser(const TSession &session)
{
    return session.value(TActionController::loginUserNameKey()).toString();
}


const QStringList &TWebSocketEndpoint::disabledEndpoints()
{
    static const QStringList disabledNames = {"application"};
    return disabledNames;
}

/*!
  Subscribes the \a topic.
  \sa publish(const QString &topic, const QString &text)
*/
void TWebSocketEndpoint::subscribe(const QString &topic, bool local)
{
    QVariantList subinfo;
    subinfo << topic << local;
    taskList << qMakePair((int)Subscribe, QVariant(subinfo));
}

/*!
  Unsubscribes the \a topic.
  \sa subscribe(const QString &topic, bool local)
*/
void TWebSocketEndpoint::unsubscribe(const QString &topic)
{
    taskList << qMakePair((int)Unsubscribe, QVariant(topic));
}

/*!
  Unsubscribes all subscribing topics.
  \sa unsubscribe(const QString &topic)
*/
void TWebSocketEndpoint::unsubscribeFromAll()
{
    taskList << qMakePair((int)UnsubscribeFromAll, QVariant());
}

/*!
  Publishes the \a text message to all subscribers of the \a topic.
  \sa subscribe(const QString &topic, bool local)
*/
void TWebSocketEndpoint::publish(const QString &topic, const QString &text)
{
    QVariantList message;
    message << topic << text;
    taskList << qMakePair((int)PublishText, QVariant(message));
}

/*!
  Publishes the \a binary message to subscribers of \a topic.
  \sa subscribe(const QString &topic, bool local)
*/
void TWebSocketEndpoint::publish(const QString &topic, const QByteArray &binary)
{
    QVariantList message;
    message << topic << binary;
    taskList << qMakePair((int)PublishBinary, QVariant(message));
}

/*!
  Starts pinging to this WebSocket. When the interval time expires without
  communication, sends Ping frame over the connection to check that the link
  is operating.
*/
void TWebSocketEndpoint::startKeepAlive(int interval)
{
    if (interval > 0) {
        taskList << qMakePair((int)StartKeepAlive, QVariant(interval));
    } else {
        taskList << qMakePair((int)StopKeepAlive, QVariant(0));
    }
}

/*!
  Sends the given \a data over the HTTP socket of the \a id.
*/
void TWebSocketEndpoint::sendHttp(int id, const QByteArray &data)
{
    QVariantList info;
    info << id << data;
    taskList << qMakePair((int)HttpSend, QVariant(info));
}


/*!
  \fn const TWebSocketSession &TWebSocketEndpoint::session() const
  Returns the current WebSocket session, allows associating information
  with individual visitors.
*/

/*!
  \fn TWebSocketSession &TWebSocketEndpoint::session()
  Returns the current WebSocket session, allows associating information
  with individual visitors.
*/

/*!
  \fn QByteArray TWebSocketEndpoint::socketId() const
  Returns the ID of this socket.
*/

/*!
  \fn QString TWebSocketEndpoint::className() const
  Returns the class name.
*/

/*!
  \fn void TWebSocketEndpoint::rollbackTransaction()
  This function is called to rollback a transaction on the database.
*/

/*!
  \fn bool TWebSocketEndpoint::transactionEnabled() const
  Must be overridden by subclasses to disable transaction mechanism.
  The function must return false to disable the mechanism. This function
  returns true.
*/
