/* Copyright (c) 2013-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tepollhttpsocket.h"
#include "tactionworker.h"
#include "tepoll.h"
#include "tepollwebsocket.h"
#include "twebsocket.h"
#include <TAppSettings>
#include <THttpRequestHeader>
#include <TSystemGlobal>
#include <TWebApplication>
#include <ctime>
using namespace Tf;

constexpr int BUFFER_RESERVE_SIZE = 1023;

namespace {
qint64 systemLimitBodyBytes = -1;
}


TEpollHttpSocket::TEpollHttpSocket(int socketDescriptor, const QHostAddress &address) :
    TEpollSocket(socketDescriptor, address),
    idleElapsed()
{
    httpBuffer.reserve(BUFFER_RESERVE_SIZE);
    idleElapsed = std::time(nullptr);
}


TEpollHttpSocket::~TEpollHttpSocket()
{
    tSystemDebug("~TEpollHttpSocket");
}


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


int TEpollHttpSocket::send()
{
    int ret = TEpollSocket::send();
    if (ret == 0) {
        idleElapsed = std::time(nullptr);
    }
    return ret;
}


int TEpollHttpSocket::recv()
{
    int ret = TEpollSocket::recv();
    if (ret == 0) {
        idleElapsed = std::time(nullptr);
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
        if (systemLimitBodyBytes > 0 && httpBuffer.length() > systemLimitBodyBytes) {
            httpBuffer.resize(0);
            throw ClientErrorException(Tf::RequestEntityTooLarge);  // Request Entity Too Large
        }

        lengthToRead = qMax(lengthToRead - pos, 0LL);
    }

    // WebSocket?
    if (lengthToRead == 0) {
        // Check connection header
        THttpRequestHeader header(httpBuffer);
        QByteArray connectionHeader = header.rawHeader("Connection").toLower();
        if (connectionHeader.contains("upgrade")) {
            QByteArray upgradeHeader = header.rawHeader("Upgrade").toLower();
            tSystemDebug("Upgrade: %s", upgradeHeader.data());

            if (upgradeHeader == "websocket") {
                if (TWebSocket::searchEndpoint(header)) {
                    // Switch protocols
                    switchToWebSocket(header);
                } else {
                    // WebSocket closing
                    disconnect();
                }
            }
            clear();  // buffer clear
        }
    }

    return true;
}


void TEpollHttpSocket::startWorker()
{
    tSystemDebug("TEpollHttpSocket::startWorker");
    TActionWorker::instance()->start(this);
    releaseWorker();
}


void TEpollHttpSocket::releaseWorker()
{
    tSystemDebug("TEpollHttpSocket::releaseWorker");

    if (pollIn.exchange(false)) {
        TEpoll::instance()->modifyPoll(this, (EPOLLIN | EPOLLOUT | EPOLLET));  // reset
    }
}


void TEpollHttpSocket::parse()
{
    if (Q_UNLIKELY(systemLimitBodyBytes < 0)) {
        systemLimitBodyBytes = Tf::appSettings()->value(Tf::LimitRequestBody, "0").toLongLong() * 2;
    }

    if (Q_LIKELY(lengthToRead < 0)) {
        int idx = httpBuffer.indexOf(CRLFCRLF);
        if (idx > 0) {
            THttpRequestHeader header(httpBuffer);
            tSystemDebug("content-length: %lld", header.contentLength());

            if (systemLimitBodyBytes > 0 && header.contentLength() > systemLimitBodyBytes) {
                httpBuffer.resize(0);
                throw ClientErrorException(Tf::RequestEntityTooLarge);  // Request EhttpBuffery Too Large
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
    httpBuffer.resize(0);
}


TEpollHttpSocket *TEpollHttpSocket::searchSocket(int sid)
{
    TEpollSocket *sock = TEpollSocket::searchSocket(sid);
    return dynamic_cast<TEpollHttpSocket *>(sock);
}


QList<TEpollHttpSocket *> TEpollHttpSocket::allSockets()
{
    QList<TEpollHttpSocket *> lst;
    for (auto sock : (const QList<TEpollSocket *> &)TEpollSocket::allSockets()) {
        auto p = dynamic_cast<TEpollHttpSocket *>(sock);
        if (p) {
            lst.append(p);
        }
    }
    return lst;
}

/*!
   Returns the number of seconds of idle time.
*/
int TEpollHttpSocket::idleTime() const
{
    return (uint)std::time(nullptr) - idleElapsed;
}
