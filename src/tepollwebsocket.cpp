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
#include "tepollwebsocket.h"

const int BUFFER_RESERVE_SIZE = 1023;
const QByteArray saltToken = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";


TEpollWebSocket::TEpollWebSocket(int socketDescriptor, const QHostAddress &address, const THttpRequestHeader &header)
    : TEpollSocket(socketDescriptor, address), reqHeader(header), lengthToRead(-1)
{
    httpBuffer.reserve(BUFFER_RESERVE_SIZE);
}


TEpollWebSocket::~TEpollWebSocket()
{ }


bool TEpollWebSocket::canReadRequest()
{
    return (lengthToRead == 0);
}


QByteArray TEpollWebSocket::readRequest()
{
    QByteArray ret = httpBuffer;
    clear();
    return ret;
}


void *TEpollWebSocket::getRecvBuffer(int )
{
    tSystemWarn("# getRecvBuffer");
    return 0;
}


bool TEpollWebSocket::seekRecvBuffer(int )
{
    tSystemWarn("# seekRecvBuffer");
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
    httpBuffer.truncate(0);
    httpBuffer.reserve(BUFFER_RESERVE_SIZE);
}


THttpResponseHeader TEpollWebSocket::handshakeResponse() const
{
    THttpResponseHeader response;
    response.setStatusLine(Tf::SwitchingProtocols);
    response.setRawHeader("Upgrade", "websocket");
    response.setRawHeader("Connection", "Upgrade");
    response.setRawHeader("Sec-WebSocket-Accept", secWebSocketAcceptString());
    return response;
}


QByteArray TEpollWebSocket::secWebSocketAcceptString() const
{
    return QCryptographicHash::hash(reqHeader.rawHeader("Sec-WebSocket-Key") + saltToken,
                                    QCryptographicHash::Sha1).toHex();
}
