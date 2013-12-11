/* Copyright (c) 2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <sys/types.h>
#include <sys/epoll.h>
#include "tepoll.h"
#include "tepollsocket.h"
#include "tsystemglobal.h"
#include "tfcore_unix.h"

const int MaxEvents = 128;

static TEpoll *staticInstance;


TEpoll::TEpoll()
    : epollFd(0), events(new struct epoll_event[MaxEvents]),
      polling(false), numEvents(0), eventIterator(0)
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

    if (numEvents < 0) {
        tSystemError("Failed epoll_wait() : errno:%d", err);
    }

    return numEvents;
}


TEpollSocket *TEpoll::next()
{
    return (eventIterator < numEvents) ? (TEpollSocket *)events[eventIterator++].data.ptr : 0;
}


bool TEpoll::canSend() const
{
    if (eventIterator <= 0)
        return false;

    return (events[eventIterator - 1].events & EPOLLOUT);
}


bool TEpoll::canReceive() const
{
    if (eventIterator <= 0)
        return false;

    return (events[eventIterator - 1].events & EPOLLIN);
}


TEpoll *TEpoll::instance()
{
    if (!staticInstance) {
        staticInstance = new TEpoll();
    }
    return staticInstance;
}


bool TEpoll::addPoll(TEpollSocket *socket, int events)
{
    if (!events)
        return false;

    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events  = events;
    ev.data.ptr = socket;

    int ret = tf_epoll_ctl(epollFd, EPOLL_CTL_ADD, socket->socketDescriptor(), &ev);
    int err = errno;
    if (ret < 0){
        if (err != EEXIST) {
            tSystemError("Failed epoll_ctl (EPOLL_CTL_ADD)  sd:%d errno:%d", socket->socketDescriptor(), err);
        }
    } else {
        tSystemDebug("OK epoll_ctl (EPOLL_CTL_ADD) (events:%d)  sd:%d", events, socket->socketDescriptor());
    }
    return !ret;

}


bool TEpoll::modifyPoll(TEpollSocket *socket, int events)
{
   if (!events)
        return false;

    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = events;
    ev.data.ptr = socket;

    int ret = tf_epoll_ctl(epollFd, EPOLL_CTL_MOD, socket->socketDescriptor(), &ev);
    int err = errno;
    if (ret < 0) {
        tSystemError("Failed epoll_ctl (EPOLL_CTL_MOD)  sd:%d errno:%d ev:0x%x", socket->socketDescriptor(), err, events);
    } else {
        tSystemDebug("OK epoll_ctl (EPOLL_CTL_MOD)  sd:%d", socket->socketDescriptor());
    }
    return !ret;

}


bool TEpoll::deletePoll(TEpollSocket *socket)
{
    int ret = tf_epoll_ctl(epollFd, EPOLL_CTL_DEL, socket->socketDescriptor(), NULL);
    int err = errno;

    if (ret < 0 && err != ENOENT) {
        tSystemError("Failed epoll_ctl (EPOLL_CTL_DEL)  sd:%d errno:%d", socket->socketDescriptor(), err);
    } else {
        tSystemDebug("OK epoll_ctl (EPOLL_CTL_DEL)  sd:%d", socket->socketDescriptor());
    }

    return !ret;
}
