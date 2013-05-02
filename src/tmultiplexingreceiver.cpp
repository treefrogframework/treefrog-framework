/* Copyright (c) 2012-2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QtGlobal>
#ifdef Q_OS_LINUX

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <QMap>
#include <QByteArray>
#include <TSystemGlobal>
#include "tmultiplexingreceiver.h"


static void setNonBlocking(int sock)
{
    int flag = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flag | O_NONBLOCK);
}


TMultiplexingReceiver::TMultiplexingReceiver(QObject *parent)
    : QThread(parent), listenSocket(0)
{ }


TMultiplexingReceiver::~TMultiplexingReceiver()
{
    if (listenSocket > 0)
        TF_CLOSE(listenSocket);
}


bool TMultiplexingReceiver::setSocketDescriptor(int socket)
{
    if (socket <= 0 || listenSocket > 0)
        return false;

    listenSocket = socket;

    start(); // starts this thread
    return true;
}


void TMultiplexingReceiver::run()
{
    const int MAX_EVENTS = 1024;
    const int BUFFER_SIZE = 2048;
    QMap<int, QByteArray> bufferings;

    int epfd = epoll_create(MAX_EVENTS);
    if (epfd < 0) {
        tSystemError("Failed epoll_create()");
        return;
    }

    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.fd = listenSocket;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, listenSocket, &ev) < 0) {
        tSystemError("Failed epoll_ctl()");
        TF_CLOSE(epfd);
        return;
    }

    struct epoll_event events[MAX_EVENTS];
    char buffer[BUFFER_SIZE];

    for (;;) {
        // Receiving/Incoming
        int nfd = epoll_wait(epfd, events, MAX_EVENTS, 20); // 20ms
        if (nfd < 0) {
            tSystemError("Failed epoll_wait()");
            break;
        }

        for (int i = 0; i < nfd; ++i) {
            if (events[i].data.fd == listenSocket) {
                // Incoming connection
                struct sockaddr_in addr;
                socklen_t addrlen = sizeof(addr);
                int clt = ::accept(events[i].data.fd, (struct sockaddr *)&addr, &addrlen);
                if (clt < 0) {
                    tSystemWarn("Failed accept");
                    continue;
                }

                setNonBlocking(clt);
                memset(&ev, 0, sizeof ev);
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = clt;
                epoll_ctl(epfd, EPOLL_CTL_ADD, clt, &ev);

            } else {
                int clt = events[i].data.fd;
                int len = ::recv(clt, buffer, BUFFER_SIZE, 0);

                if (len <= 0) {
                    tSystemError("Failed read : %d", len);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, clt, &ev);
                    bufferings.remove(clt);
                    TF_CLOSE(clt);
                } else {
                    // Read successfully
                    QByteArray &buf = bufferings[clt];
                    buf.append(buffer, len);
                    int idx = buf.indexOf("\r\n\r\n");

                    if (idx > 0) {
                        incomingRequest(clt, buf);
                        epoll_ctl(epfd, EPOLL_CTL_DEL, clt, &ev);
                        bufferings.remove(clt);
                    }
                }
            }
        }

        // Check stop flag
        // if (..) ...
    }

    TF_CLOSE(epfd);
    TF_CLOSE(listenSocket);
    listenSocket = 0;
}

#endif // Q_OS_LINUX
