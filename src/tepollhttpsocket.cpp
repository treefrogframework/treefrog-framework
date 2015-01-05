/* Copyright (c) 2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TSystemGlobal>
#include <TAppSettings>
#include <THttpRequestHeader>
#include "tepollhttpsocket.h"
#include "tactionworker.h"

const int BUFFER_RESERVE_SIZE = 1023;
static int limitBodyBytes = -1;


TEpollHttpSocket::TEpollHttpSocket(int socketDescriptor, int id, const QHostAddress &address)
    : TEpollSocket(socketDescriptor, id, address), lengthToRead(-1)
{
    httpBuffer.reserve(BUFFER_RESERVE_SIZE);
}


TEpollHttpSocket::~TEpollHttpSocket()
{ }


bool TEpollHttpSocket::canReadRequest()
{
    return (lengthToRead == 0);
}


QByteArray TEpollHttpSocket::readRequest()
{
    QByteArray ret = httpBuffer;
    clear();
    return ret;
}


bool TEpollHttpSocket::upgradeConnectionReceived() const
{
    return false;
}


TEpollSocket *TEpollHttpSocket::switchProtocol()
{
    return NULL;
}


int TEpollHttpSocket::write(const char *data, int len)
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


void TEpollHttpSocket::startWorker()
{
    TActionWorker *worker = new TActionWorker(this);
    connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
    worker->start();
}


void TEpollHttpSocket::parse()
{
    if (Q_UNLIKELY(limitBodyBytes < 0)) {
        limitBodyBytes = Tf::appSettings()->value(Tf::LimitRequestBody, "0").toInt();
    }

    if (Q_LIKELY(lengthToRead < 0)) {
        int idx = httpBuffer.indexOf("\r\n\r\n");
        if (idx > 0) {
            THttpRequestHeader header(httpBuffer);
            tSystemDebug("content-length: %d", header.contentLength());

            if (limitBodyBytes > 0 && header.contentLength() > (uint)limitBodyBytes) {
                throw ClientErrorException(413);  // Request Entity Too Large
            }

            lengthToRead = qMax(idx + 4 + (qint64)header.contentLength() - httpBuffer.length(), 0LL);
            tSystemDebug("lengthToRead: %d", (int)lengthToRead);
        }
    } else {
        tSystemWarn("Unreachable code in normal communication");
    }
}


void TEpollHttpSocket::clear()
{
    lengthToRead = -1;
    httpBuffer.truncate(0);
    httpBuffer.reserve(BUFFER_RESERVE_SIZE);
}
