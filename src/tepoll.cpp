/* Copyright (c) 2013-2017, AOYAMA Kazuharu
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
#include <TWebApplication>
#include <THttpRequestHeader>
#include <TApplicationServerBase>
#include <TSession>
#include "tepoll.h"
#include "tepollsocket.h"
#include "tsendbuffer.h"
#include "tepollwebsocket.h"
#include "tsessionmanager.h"
#include "tsystemglobal.h"
#include "tfcore.h"

const int MaxEvents = 128;

static TEpoll *staticInstance;


class TSendData
{
public:
    enum Method {
        Disconnect,
        Send,
        SwitchToWebSocket,
    };

    int method;
    TEpollSocket *socket;
    TSendBuffer *buffer;
    THttpRequestHeader header;

    TSendData(Method m, TEpollSocket *s, TSendBuffer *buf = 0)
        : method(m), socket(s), buffer(buf), header()
    { }

    TSendData(Method m, TEpollSocket *s, const THttpRequestHeader &h)
        : method(m), socket(s), buffer(0), header(h)
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
        tf_close(epollFd);
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
    if (Q_UNLIKELY(!events)) {
        return false;
    }
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
        tSystemDebug("OK epoll_ctl (EPOLL_CTL_ADD) (events:%u)  sd:%d", events, socket->socketDescriptor());
        pollingSockets.insert(socket, socket->socketId());
    }
    return !ret;
}


bool TEpoll::modifyPoll(TEpollSocket *socket, int events)
{
    if (Q_UNLIKELY(!events)) {
        return false;
    }
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
    if (Q_UNLIKELY(pollingSockets.remove(socket) == 0)) {
        return false;
    }

    int ret = tf_epoll_ctl(epollFd, EPOLL_CTL_DEL, socket->socketDescriptor(), nullptr);
    int err = errno;

    if (Q_UNLIKELY(ret < 0 && err != ENOENT)) {
        tSystemError("Failed epoll_ctl (EPOLL_CTL_DEL)  sd:%d errno:%d", socket->socketDescriptor(), err);
    } else {
        tSystemDebug("OK epoll_ctl (EPOLL_CTL_DEL)  sd:%d", socket->socketDescriptor());
    }

    return !ret;
}


void TEpoll::dispatchSendData()
{
    TSendData *sd;
    while (sendRequests.dequeue(sd)) {
        TEpollSocket *sock = sd->socket;

        if (Q_UNLIKELY(sock->socketDescriptor() <= 0)) {
            tSystemDebug("already disconnected:  sid:%d", sock->socketId());
            continue;
        }

        switch (sd->method) {
        case TSendData::Disconnect:
            deletePoll(sock);
            sock->close();
            sock->deleteLater();
            break;

        case TSendData::SwitchToWebSocket: {
            tSystemDebug("Switch to WebSocket");
            Q_ASSERT(sd->buffer == nullptr);

            QByteArray secKey = sd->header.rawHeader("Sec-WebSocket-Key");
            tSystemDebug("secKey: %s", secKey.data());
            int newsocket = TApplicationServerBase::duplicateSocket(sock->socketDescriptor());

            // Switch to WebSocket
            TEpollWebSocket *ws = new TEpollWebSocket(newsocket, sock->peerAddress(), sd->header);
            ws->moveToThread(Tf::app()->thread());
            addPoll(ws, (EPOLLIN | EPOLLOUT | EPOLLET));  // reset

            // Stop polling and delete
            deletePoll(sock);
            sock->deleteLater();

            // WebSocket opening
            TSession session;
            QByteArray sessionId = sd->header.cookie(TSession::sessionName());
            if (!sessionId.isEmpty()) {
                // Finds a session
                session = TSessionManager::instance().findSession(sessionId);
            }
            ws->startWorkerForOpening(session);
            break; }

        default:
            tSystemError("Logic error [%s:%d]", __FILE__, __LINE__);
            delete sd->buffer;
            break;
        }

        delete sd;
    }
}


void TEpoll::releaseAllPollingSockets()
{
    for (QMapIterator<TEpollSocket *, int> it(pollingSockets); it.hasNext(); ) {
        it.next();
        it.key()->deleteLater();
    }
    pollingSockets.clear();
}


void TEpoll::setSendData(TEpollSocket *socket, const QByteArray &header, QIODevice *body, bool autoRemove, const TAccessLogger &accessLogger)
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

    TSendBuffer *sendbuf = TEpollSocket::createSendBuffer(response, fi, autoRemove, accessLogger);
    socket->enqueueSendData(sendbuf);
    modifyPoll(socket, (EPOLLIN | EPOLLOUT | EPOLLET));  // reset
}


void TEpoll::setSendData(TEpollSocket *socket, const QByteArray &data)
{
    TSendBuffer *sendbuf = TEpollSocket::createSendBuffer(data);
    socket->enqueueSendData(sendbuf);
    modifyPoll(socket, (EPOLLIN | EPOLLOUT | EPOLLET));  // reset
}


void TEpoll::setDisconnect(TEpollSocket *socket)
{
    sendRequests.enqueue(new TSendData(TSendData::Disconnect, socket));
}


void TEpoll::setSwitchToWebSocket(TEpollSocket *socket, const THttpRequestHeader &header)
{
    sendRequests.enqueue(new TSendData(TSendData::SwitchToWebSocket, socket, header));
}
