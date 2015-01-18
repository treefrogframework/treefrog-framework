/* Copyright (c) 2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QCryptographicHash>
#include <TSystemGlobal>
#include <TAppSettings>
#include <THttpRequestHeader>
#include <THttpUtility>
#include "tepollwebsocket.h"

const int BUFFER_RESERVE_SIZE = 1023;
const QByteArray saltToken = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";


TEpollWebSocket::TEpollWebSocket(int socketDescriptor, const QHostAddress &address, const THttpRequestHeader &header)
    : TEpollSocket(socketDescriptor, address), uuid(QUuid::createUuid()), reqHeader(header),
      recvBuffer(), lengthToRead(-1)
{
    recvBuffer.reserve(BUFFER_RESERVE_SIZE);
}


TEpollWebSocket::~TEpollWebSocket()
{ }


void TEpollWebSocket::sendText(const QString &message)
{
    sendText(socketUuid(), message);
}


void TEpollWebSocket::sendBinary(const QByteArray &data)
{
    sendBinary(socketUuid(), data);
}


bool TEpollWebSocket::canReadRequest()
{
    return (lengthToRead == 0);
}


QByteArray TEpollWebSocket::readRequest()
{
    QByteArray ret = recvBuffer;
    clear();
    return ret;
}


void *TEpollWebSocket::getRecvBuffer(int size)
{
    recvBuffer.reserve(size);
    return recvBuffer.data();
}


bool TEpollWebSocket::seekRecvBuffer(int pos)
{
    tSystemWarn("# seekRecvBuffer: pos:%d", pos);
    return true;
}


void TEpollWebSocket::startWorker()
{
    // TActionWorker *worker = new TActionWorker(this);
    // connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
    // worker->start();
}


void TEpollWebSocket::parse()
{

}


void TEpollWebSocket::clear()
{
    lengthToRead = -1;
    recvBuffer.truncate(0);
    recvBuffer.reserve(BUFFER_RESERVE_SIZE);
}


THttpResponseHeader TEpollWebSocket::handshakeResponse() const
{
    THttpResponseHeader response;
    response.setStatusLine(Tf::SwitchingProtocols, THttpUtility::getResponseReasonPhrase(Tf::SwitchingProtocols));
    response.setRawHeader("Upgrade", "websocket");
    response.setRawHeader("Connection", "Upgrade");

    QByteArray secAccept = QCryptographicHash::hash(reqHeader.rawHeader("Sec-WebSocket-Key").trimmed() + saltToken,
                                                    QCryptographicHash::Sha1).toBase64();
    response.setRawHeader("Sec-WebSocket-Accept", secAccept);
    return response;
}
