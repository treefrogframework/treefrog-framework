/* Copyright (c) 2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QByteArray>
#include <QFileInfo>
#include <QBuffer>
#include <sys/types.h>
#include <sys/epoll.h>
#include "tepoll.h"
#include "tepollsocket.h"
#include "thttpsendbuffer.h"
#include "tsystemglobal.h"
#include "tfcore_unix.h"

const int MaxEvents = 128;

static TEpoll *staticInstance;


class TSendData
{
public:
    enum Method {
        Disconnect,
        Send,
        SwitchProtocols,
    };
    int method;
    quint64  id;
    THttpSendBuffer *buffer;
    TEpollSocket *upgradedSocket;

    TSendData(Method m, quint64 i, THttpSendBuffer *buf = 0, TEpollSocket *upgraded = 0)
        : method(m), id(i), buffer(buf), upgradedSocket(upgraded)
    { }
};



TEpoll::TEpoll()
    : epollFd(0), events(new struct epoll_event[MaxEvents]),
      polling(false), numEvents(0), eventIterator(0), pollingSockets()
{
    epollFd = epoll_create(1);
    if (epollFd < 0) {
        tSystemError("Failed epoll_create()");
    }
}


TEpoll::~TEpoll()
{
    delete events;

    if (epollFd > 0)
        TF_CLOSE(epollFd);
}


int TEpoll::wait(int timeout)
{
    eventIterator = 0;
    polling = true;
    numEvents = tf_epoll_wait(epollFd, events, MaxEvents, timeout);
    int err = errno;
    polling = false;

    if (Q_UNLIKELY(numEvents < 0)) {
        tSystemError("Failed epoll_wait() : errno:%d", err);
    }

    return numEvents;
}


TEpollSocket *TEpoll::next()
{
    return (eventIterator < numEvents) ? (TEpollSocket *)events[eventIterator++].data.ptr : 0;
}

bool TEpoll::canReceive() const
{
    if (Q_UNLIKELY(eventIterator <= 0))
        return false;

    return (events[eventIterator - 1].events & EPOLLIN);
}


bool TEpoll::canSend() const
{
    if (Q_UNLIKELY(eventIterator <= 0))
        return false;

    return (events[eventIterator - 1].events & EPOLLOUT);
}


int TEpoll::recv(TEpollSocket *socket) const
{
    return socket->recv();
}


int TEpoll::send(TEpollSocket *socket) const
{
    return socket->send();
}


TEpoll *TEpoll::instance()
{
    if (Q_UNLIKELY(!staticInstance)) {
        staticInstance = new TEpoll();
    }
    return staticInstance;
}


bool TEpoll::addPoll(TEpollSocket *socket, int events)
{
    if (Q_UNLIKELY(!events))
        return false;

    struct epoll_event ev;
    ev.events  = events;
    ev.data.ptr = socket;

    int ret = tf_epoll_ctl(epollFd, EPOLL_CTL_ADD, socket->socketDescriptor(), &ev);
    int err = errno;
    if (Q_UNLIKELY(ret < 0)){
        if (err != EEXIST) {
            tSystemError("Failed epoll_ctl (EPOLL_CTL_ADD)  sd:%d errno:%d", socket->socketDescriptor(), err);
        }
    } else {
        tSystemDebug("OK epoll_ctl (EPOLL_CTL_ADD) (events:%d)  sd:%d", events, socket->socketDescriptor());
        pollingSockets.insert(socket->objectId(), socket);
    }
    return !ret;

}


bool TEpoll::modifyPoll(TEpollSocket *socket, int events)
{
   if (!events)
        return false;

    struct epoll_event ev;
    ev.events = events;
    ev.data.ptr = socket;

    int ret = tf_epoll_ctl(epollFd, EPOLL_CTL_MOD, socket->socketDescriptor(), &ev);
    int err = errno;
    if (Q_UNLIKELY(ret < 0)) {
        tSystemError("Failed epoll_ctl (EPOLL_CTL_MOD)  sd:%d errno:%d ev:0x%x", socket->socketDescriptor(), err, events);
    } else {
        tSystemDebug("OK epoll_ctl (EPOLL_CTL_MOD)  sd:%d", socket->socketDescriptor());
    }
    return !ret;

}


bool TEpoll::deletePoll(TEpollSocket *socket)
{
    pollingSockets.remove(socket->objectId());

    int ret = tf_epoll_ctl(epollFd, EPOLL_CTL_DEL, socket->socketDescriptor(), NULL);
    int err = errno;

    if (Q_UNLIKELY(ret < 0 && err != ENOENT)) {
        tSystemError("Failed epoll_ctl (EPOLL_CTL_DEL)  sd:%d errno:%d", socket->socketDescriptor(), err);
    } else {
        tSystemDebug("OK epoll_ctl (EPOLL_CTL_DEL)  sd:%d", socket->socketDescriptor());
    }

    return !ret;
}


bool TEpoll::waitSendData(int msec)
{
    return sendRequests.wait(msec);
}


void TEpoll::dispatchSendData()
{
    QList<TSendData *> dataList = sendRequests.dequeue();

    for (QListIterator<TSendData *> it(dataList); it.hasNext(); ) {
        TSendData *sd = it.next();
        TEpollSocket *sock = pollingSockets[sd->id];

        if (Q_LIKELY(sock && sock->socketDescriptor() > 0)) {
            switch (sd->method) {
            case TSendData::Send:
                sock->sendBuf << sd->buffer;
                modifyPoll(sock, (EPOLLIN | EPOLLOUT | EPOLLET));  // reset
                break;

            case TSendData::Disconnect:
                deletePoll(sock);
                sock->close();
                sock->deleteLater();
                break;

            case TSendData::SwitchProtocols:
                deletePoll(sock);
                sock->setSocketDescpriter(0);  // Delegates to new websocket
                sock->deleteLater();
    tSystemDebug("##### SwitchProtocols");
                // Switching protocols
                sd->upgradedSocket->sendBuf << sd->buffer;
                addPoll(sd->upgradedSocket, (EPOLLIN | EPOLLOUT | EPOLLET));  // reset
                break;

            default:
                tSystemError("Logic error [%s:%d]", __FILE__, __LINE__);
                if (sd->buffer) {
                    delete sd->buffer;
                }
                break;
            }
        }

        delete sd;
    }
}


void TEpoll::releaseAllPollingSockets()
{
    for (QMapIterator<quint64, TEpollSocket *> it(pollingSockets); it.hasNext(); ) {
        it.next();
        it.value()->deleteLater();
    }
    pollingSockets.clear();
}


void TEpoll::setSendData(quint64 id, const QByteArray &header, QIODevice *body, bool autoRemove, const TAccessLogger &accessLogger)
{
    QByteArray response = header;
    QFileInfo fi;

    if (Q_LIKELY(body)) {
        QBuffer *buffer = qobject_cast<QBuffer *>(body);
        if (buffer) {
            response += buffer->data();
        } else {
            fi.setFile(*qobject_cast<QFile *>(body));
        }
    }

    THttpSendBuffer *sendbuf = new THttpSendBuffer(response, fi, autoRemove, accessLogger);
    sendRequests.enqueue(new TSendData(TSendData::Send, id, sendbuf));
}


void TEpoll::setDisconnect(quint64 id)
{
    sendRequests.enqueue(new TSendData(TSendData::Disconnect, id));
}


void TEpoll::setSwitchProtocols(quint64 id, const QByteArray &header, TEpollSocket *target)
{
    THttpSendBuffer *sendbuf = new THttpSendBuffer(header);
    sendRequests.enqueue(new TSendData(TSendData::SwitchProtocols, id, sendbuf, target));
}

