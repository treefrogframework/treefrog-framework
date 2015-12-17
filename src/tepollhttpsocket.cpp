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
#include "tepollwebsocket.h"
#include "twebsocket.h"

const int BUFFER_RESERVE_SIZE = 1023;
static int limitBodyBytes = -1;
static QMutex mutexMap;
static QMap<QByteArray, TEpollHttpSocket*> socketMap;


TEpollHttpSocket::TEpollHttpSocket(int socketDescriptor, const QHostAddress &address)
    : TEpollSocket(socketDescriptor, address), lengthToRead(-1), idleElapsed()
{
    httpBuffer.reserve(BUFFER_RESERVE_SIZE);
    idleElapsed.start();

    mutexMap.lock();
    socketMap.insert(socketUuid(), this);
    mutexMap.unlock();
}


TEpollHttpSocket::~TEpollHttpSocket()
{
    tSystemDebug("~TEpollHttpSocket");

    mutexMap.lock();
    socketMap.remove(socketUuid());
    mutexMap.unlock();
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
        idleElapsed.start();
    }
    return ret;
}


int TEpollHttpSocket::recv()
{
    int ret = TEpollSocket::recv();
    if (ret == 0) {
        idleElapsed.start();
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

    TActionWorker *worker = new TActionWorker(this);
    worker->moveToThread(Tf::app()->thread());
    connect(worker, SIGNAL(finished()), this, SLOT(releaseWorker()));
    ++myWorkerCounter; // count-up
    worker->start();
}


void TEpollHttpSocket::releaseWorker()
{
    tSystemDebug("TEpollHttpSocket::releaseWorker");

    TActionWorker *worker = qobject_cast<TActionWorker *>(sender());
    if (worker) {
        worker->deleteLater();
        --myWorkerCounter;  // count-down

        if (deleting.load()) {
            TEpollSocket::deleteLater();
        } else {
            if (pollIn.exchange(false)) {
                TEpoll::instance()->modifyPoll(this, (EPOLLIN | EPOLLOUT | EPOLLET));  // reset
            }
        }
    }
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


void TEpollHttpSocket::deleteLater()
{
    tSystemDebug("TEpollHttpSocket::deleteLater  countWorker:%d", (int)myWorkerCounter);
    deleting = true;
    if ((int)myWorkerCounter == 0) {
        mutexMap.lock();
        socketMap.remove(socketUuid());
        mutexMap.unlock();

        QObject::deleteLater();
    }
}


TEpollHttpSocket *TEpollHttpSocket::searchSocket(const QByteArray &uuid)
{
    QMutexLocker locker(&mutexMap);
    return socketMap.value(uuid, nullptr);
}


QList<TEpollHttpSocket*> TEpollHttpSocket::allSockets()
{
    QMutexLocker locker(&mutexMap);
    return socketMap.values();
}
