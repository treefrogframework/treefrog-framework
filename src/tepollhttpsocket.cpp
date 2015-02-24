/* Copyright (c) 2013-2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TWebApplication>
#include <TSystemGlobal>
#include <TAppSettings>
#include <THttpRequestHeader>
#include "tepollhttpsocket.h"
#include "tactionworker.h"
#include "tepoll.h"

const int BUFFER_RESERVE_SIZE = 1023;
static int limitBodyBytes = -1;


TEpollHttpSocket::TEpollHttpSocket(int socketDescriptor, const QHostAddress &address)
    : TEpollSocket(socketDescriptor, address), lengthToRead(-1)
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
    QByteArray ret;
    if (canReadRequest()) {
        ret = httpBuffer;
        clear();
    }
    return ret;
}


void *TEpollHttpSocket::getRecvBuffer(int size)
{
    int len = httpBuffer.size();
    httpBuffer.reserve(len + size);
    return httpBuffer.data() + len;
}


bool TEpollHttpSocket::seekRecvBuffer(int pos)
{
    int len = httpBuffer.size();
    if (Q_UNLIKELY(pos <= 0 || len + pos > httpBuffer.capacity())) {
        return false;
    }

    len += pos;
    httpBuffer.resize(len);

    if (lengthToRead < 0) {
        parse();
    } else {
        if (limitBodyBytes > 0 && httpBuffer.length() > limitBodyBytes) {
            throw ClientErrorException(413);  // Request Entity Too Large
        }

        lengthToRead = qMax(lengthToRead - pos, 0LL);
    }
    return true;
}


void TEpollHttpSocket::startWorker()
{
    TActionWorker *worker = new TActionWorker(this);
    worker->moveToThread(Tf::app()->thread());
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

            // Check connection header
            QByteArray connectionHeader = header.rawHeader("Connection").toLower();
            if (connectionHeader.contains("upgrade")) {
                QByteArray upgradeHeader = header.rawHeader("Upgrade").toLower();
                tSystemDebug("Upgrade: %s", upgradeHeader.data());

                if (upgradeHeader == "websocket") {
                    // Switch protocols
                    TEpoll::instance()->setSwitchToWebSocket(socketUuid(), header);
                }
            }
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
