/* Copyright (c) 2013-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tepoll.h"
#include "tepollsocket.h"
#include "tepollwebsocket.h"
#include "tfcore.h"
#include "tsendbuffer.h"
#include "tsessionmanager.h"
#include "tsystemglobal.h"
#include <QBuffer>
#include <QByteArray>
#include <QFileInfo>
#include <TApplicationServerBase>
#include <THttpRequestHeader>
#include <TSession>
#include <TWebApplication>
#include <sys/epoll.h>
#include <sys/types.h>

constexpr int MaxEvents = 128;


class TSendData {
public:
    enum Method {
        Disconnect,
        Send,
        SwitchToWebSocket,
    };

    int method {Disconnect};
    TEpollSocket *socket {nullptr};
    TSendBuffer *buffer {nullptr};
    THttpRequestHeader header;

    TSendData(Method m, TEpollSocket *s, TSendBuffer *buf = 0) :
        method(m), socket(s), buffer(buf), header()
    {
    }

    TSendData(Method m, TEpollSocket *s, const THttpRequestHeader &h) :
        method(m), socket(s), buffer(0), header(h)
    {
    }
};


TEpoll::TEpoll() :
    _events(new struct epoll_event[MaxEvents])
{
    _epollFd = epoll_create1(EPOLL_CLOEXEC);
    if (_epollFd < 0) {
        tSystemError("Failed epoll_create1()");
    }
}


TEpoll::~TEpoll()
{
    delete[] _events;

    if (_epollFd > 0) {
        tf_close_socket(_epollFd);
    }
}


int TEpoll::wait(int timeout)
{
    if (_eventIterator < _numEvents) {
        return _numEvents - _eventIterator;
    }

    _eventIterator = 0;
    _polling = true;
    _numEvents = tf_epoll_wait(_epollFd, _events, MaxEvents, timeout);
    int err = errno;
    _polling = false;

    if (Q_UNLIKELY(_numEvents < 0)) {
        tSystemError("Failed epoll_wait() : errno:{}", err);
    }

    return _numEvents;
}


TEpollSocket *TEpoll::next()
{
    return (_eventIterator < _numEvents) ? (TEpollSocket *)_events[_eventIterator++].data.ptr : nullptr;
}

bool TEpoll::canReceive() const
{
    if (Q_UNLIKELY(_eventIterator <= 0))
        return false;

    return (_events[_eventIterator - 1].events & EPOLLIN);
}


bool TEpoll::canSend() const
{
    if (Q_UNLIKELY(_eventIterator <= 0)) {
        return false;
    }
    return (_events[_eventIterator - 1].events & EPOLLOUT);
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
    static TEpoll staticInstance;
    return &staticInstance;
}


bool TEpoll::addPoll(TEpollSocket *socket, int events)
{
    if (!events || socket->socketDescriptor() == 0) {
        return false;
    }
    struct epoll_event ev;
    ev.events = events;
    ev.data.ptr = socket;

    int ret = tf_epoll_ctl(_epollFd, EPOLL_CTL_ADD, socket->socketDescriptor(), &ev);
    int err = errno;
    if (Q_UNLIKELY(ret < 0)) {
        if (err != EEXIST) {
            tSystemError("Failed epoll_ctl (EPOLL_CTL_ADD)  sd:{} errno:{}", socket->socketDescriptor(), err);
        } else {
            ret = 0;
        }
    } else {
        tSystemDebug("OK epoll_ctl (EPOLL_CTL_ADD) (events:{})  sd:{}", events, socket->socketDescriptor());
    }
    return !ret;
}


bool TEpoll::modifyPoll(TEpollSocket *socket, int events)
{
    if (!events || socket->socketDescriptor() == 0) {
        return false;
    }
    struct epoll_event ev;
    ev.events = events;
    ev.data.ptr = socket;

    int ret = tf_epoll_ctl(_epollFd, EPOLL_CTL_MOD, socket->socketDescriptor(), &ev);
    int err = errno;
    if (Q_UNLIKELY(ret < 0)) {
        tSystemError("Failed epoll_ctl (EPOLL_CTL_MOD)  sd:{} errno:{} ev:{:#x}", socket->socketDescriptor(), err, events);
    } else {
        tSystemDebug("OK epoll_ctl (EPOLL_CTL_MOD)  sd:{}", socket->socketDescriptor());
    }
    return !ret;
}


bool TEpoll::deletePoll(TEpollSocket *socket)
{
    if (socket->socketDescriptor() == 0) {
        return false;
    }

    int ret = tf_epoll_ctl(_epollFd, EPOLL_CTL_DEL, socket->socketDescriptor(), nullptr);
    int err = errno;

    if (Q_UNLIKELY(ret < 0 && err != ENOENT)) {
        tSystemError("Failed epoll_ctl (EPOLL_CTL_DEL)  sd:{} errno:{}", socket->socketDescriptor(), err);
    } else {
        tSystemDebug("OK epoll_ctl (EPOLL_CTL_DEL)  sd:{}", socket->socketDescriptor());
    }

    return !ret;
}


void TEpoll::dispatchEvents()
{
    TSendData *sd;
    while (_sendRequests.dequeue(sd)) {
        TEpollSocket *sock = sd->socket;

        if (Q_UNLIKELY(sock->socketDescriptor() <= 0)) {
            continue;
        }

        switch (sd->method) {
        case TSendData::Disconnect:
            deletePoll(sock);
            sock->dispose();
            break;

        case TSendData::SwitchToWebSocket: {
            tSystemDebug("Switch to WebSocket");
            Q_ASSERT(sd->buffer == nullptr);

            QByteArray secKey = sd->header.rawHeader("Sec-WebSocket-Key");
            tSystemDebug("secKey: {}", secKey);
            int newsocket = TApplicationServerBase::duplicateSocket(sock->socketDescriptor());

            // Switch to WebSocket
            TEpollWebSocket *ws = new TEpollWebSocket(newsocket, sock->peerAddress(), sd->header);
            ws->moveToThread(Tf::app()->thread());
            bool res = ws->watch();
            if (!res) {
                sock->dispose();
            }

            // Stop polling and delete
            deletePoll(sock);
            sock->dispose();

            // WebSocket opening
            TSession session;
            QByteArray sessionId = sd->header.cookie(TSession::sessionName());
            if (!sessionId.isEmpty()) {
                // Finds a session
                session = TSessionManager::instance().findSession(sessionId);
            }
            ws->startWorkerForOpening(session);
            break;
        }

        default:
            tSystemError("Logic error [{}:{}]", __FILE__, __LINE__);
            delete sd->buffer;
            break;
        }

        delete sd;
    }
}


void TEpoll::releaseAllPollingSockets()
{
    auto set = TEpollSocket::allSockets();
    for (auto *socket : set) {
        if (socket->autoDelete()) {
            delete socket;
        }
    }
}


void TEpoll::setSendData(TEpollSocket *socket, const QByteArray &header, QIODevice *body, bool autoRemove, TAccessLogger &&accessLogger)
{
    QByteArray response = header;
    QFileInfo fi;

    if (Q_LIKELY(body)) {
        QBuffer *buffer = dynamic_cast<QBuffer *>(body);
        if (buffer) {
            response += buffer->data();
        } else {
            fi.setFile(*dynamic_cast<QFile *>(body));
        }
    }

    TSendBuffer *sendbuf = TEpollSocket::createSendBuffer(response, fi, autoRemove, std::move(accessLogger));
    socket->enqueueSendData(sendbuf);
    bool res = modifyPoll(socket, (EPOLLIN | EPOLLOUT | EPOLLET));  // reset
    if (!res) {
        socket->dispose();
    }
}


void TEpoll::setSendData(TEpollSocket *socket, const QByteArray &data)
{
    TSendBuffer *sendbuf = TEpollSocket::createSendBuffer(data);
    socket->enqueueSendData(sendbuf);
    bool res = modifyPoll(socket, (EPOLLIN | EPOLLOUT | EPOLLET));  // reset
    if (!res) {
        socket->dispose();
    }
}


void TEpoll::setDisconnect(TEpollSocket *socket)
{
    _sendRequests.enqueue(new TSendData(TSendData::Disconnect, socket));
}


void TEpoll::setSwitchToWebSocket(TEpollSocket *socket, const THttpRequestHeader &header)
{
    _sendRequests.enqueue(new TSendData(TSendData::SwitchToWebSocket, socket, header));
}
