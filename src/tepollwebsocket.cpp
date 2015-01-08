/* Copyright (c) 2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TSystemGlobal>
#include <TAppSettings>
#include <THttpRequestHeader>
#include "tepollwebsocket.h"
//#include "tactionworker.h"

const int BUFFER_RESERVE_SIZE = 1023;
static int limitBodyBytes = -1;


TEpollWebSocket::TEpollWebSocket(int socketDescriptor, const QHostAddress &address)
    : TEpollSocket(socketDescriptor, address), lengthToRead(-1)
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


int TEpollWebSocket::write(const char *data, int len)
{
    httpBuffer.append(data, len);

    if (lengthToRead < 0) {
        parse();
    } else {
        if (limitBodyBytes > 0 && httpBuffer.length() > limitBodyBytes) {
            throw ClientErrorException(413);  // Request Entity Too Large
        }

        lengthToRead = qMax(lengthToRead - len, 0LL);
    }
    return len;
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
