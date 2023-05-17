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
#include "tfcore.h"
#include <TAppSettings>
#include <THttpRequestHeader>
#include <TSystemGlobal>
#include <TWebApplication>
#include <ctime>
using namespace Tf;

constexpr int BUFFER_RESERVE_SIZE = 1023;

namespace {
int64_t systemLimitBodyBytes = -1;
}

TEpollHttpSocket *TEpollHttpSocket::accept(int listeningSocket)
{
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);

    int actfd = tf_accept4(listeningSocket, (sockaddr *)&addr, &addrlen, SOCK_CLOEXEC | SOCK_NONBLOCK);
    int err = errno;
    if (Q_UNLIKELY(actfd < 0)) {
        if (err != EAGAIN) {
            tSystemWarn("Failed accept.  errno:%d", err);
        }
        return nullptr;
    }

    return create(actfd, QHostAddress((sockaddr *)&addr), true);
}


TEpollHttpSocket *TEpollHttpSocket::create(int socketDescriptor, const QHostAddress &address, bool watch)
{
    TEpollHttpSocket *sock = nullptr;

    if (Q_LIKELY(socketDescriptor > 0)) {
        sock = new TEpollHttpSocket(socketDescriptor, address);
    }

    if (watch) {
        sock->watch();
    }

    return sock;
}


TEpollHttpSocket::TEpollHttpSocket(int socketDescriptor, const QHostAddress &address) :
    TEpollSocket(socketDescriptor, Tf::SocketState::Connected, address),
    _idleElapsed()
{
    _recvBuffer.reserve(BUFFER_RESERVE_SIZE);
    _idleElapsed = std::time(nullptr);
}


TEpollHttpSocket::~TEpollHttpSocket()
{
    tSystemDebug("~TEpollHttpSocket");
}


bool TEpollHttpSocket::canReadRequest()
{
    return (_lengthToRead == 0);
}


QByteArray TEpollHttpSocket::readRequest()
{
    QByteArray ret;
    if (canReadRequest()) {
        ret = _recvBuffer;
        clear();
    }
    return ret;
}


int TEpollHttpSocket::send()
{
    int ret = TEpollSocket::send();
    if (ret == 0) {
        _idleElapsed = std::time(nullptr);
    }
    return ret;
}


int TEpollHttpSocket::recv()
{
    int ret = TEpollSocket::recv();
    if (ret == 0) {
        _idleElapsed = std::time(nullptr);
    }
    return ret;
}


bool TEpollHttpSocket::seekRecvBuffer(int pos)
{
    int len = _recvBuffer.size();
    if (Q_UNLIKELY(pos <= 0 || len + pos > _recvBuffer.capacity())) {
        return false;
    }

    len += pos;
    _recvBuffer.resize(len);

    if (_lengthToRead < 0) {
        parse();
    } else {
        if (systemLimitBodyBytes > 0 && _recvBuffer.length() > systemLimitBodyBytes) {
            _recvBuffer.resize(0);
            throw ClientErrorException(Tf::RequestEntityTooLarge);  // Request Entity Too Large
        }

        _lengthToRead = std::max(_lengthToRead - pos, (int64_t)0);
    }

    // WebSocket?
    if (_lengthToRead == 0) {
        // Check connection header
        THttpRequestHeader header(_recvBuffer);
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


void TEpollHttpSocket::process()
{
    tSystemDebug("TEpollHttpSocket::process");
    _worker = new TActionWorker;
    _worker->start(this);
    delete _worker;
    _worker = nullptr;
    releaseWorker();
}


void TEpollHttpSocket::releaseWorker()
{
    tSystemDebug("TEpollHttpSocket::releaseWorker");

    bool res = TEpoll::instance()->modifyPoll(this, (EPOLLIN | EPOLLOUT | EPOLLET));  // reset
    if (!res) {
        dispose();
    }
}


void TEpollHttpSocket::parse()
{
    if (Q_UNLIKELY(systemLimitBodyBytes < 0)) {
        systemLimitBodyBytes = Tf::appSettings()->value(Tf::LimitRequestBody).toLongLong() * 2;
    }

    if (Q_LIKELY(_lengthToRead < 0)) {
        int idx = _recvBuffer.indexOf(CRLFCRLF);
        if (idx > 0) {
            THttpRequestHeader header(_recvBuffer);

            if (systemLimitBodyBytes > 0 && header.contentLength() > systemLimitBodyBytes) {
                _recvBuffer.resize(0);
                throw ClientErrorException(Tf::RequestEntityTooLarge);  // Request EhttpBuffery Too Large
            }

            _lengthToRead = std::max(idx + 4 + (int64_t)header.contentLength() - (int64_t)_recvBuffer.length(), (int64_t)0);
            tSystemDebug("lengthToRead: %d", (int)_lengthToRead);
        }
    } else {
        tSystemWarn("Unreachable code in normal communication");
    }
}


void TEpollHttpSocket::clear()
{
    _lengthToRead = -1;
    _recvBuffer.resize(0);
}


QList<TEpollHttpSocket *> TEpollHttpSocket::allSockets()
{
    QList<TEpollHttpSocket *> lst;
    auto set = TEpollSocket::allSockets();
    for (auto it = set.constBegin(); it != set.constEnd(); ++it) {
        auto p = dynamic_cast<TEpollHttpSocket *>(*it);
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
    return (uint)std::time(nullptr) - _idleElapsed;
}
