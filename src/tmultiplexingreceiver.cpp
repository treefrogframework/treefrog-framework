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
#include <stdio.h>   // For BUFSIZ
#include <errno.h>
#include <QMap>
#include <QHostAddress>
#include <QBuffer>
#include <TWebApplication>
#include <TApplicationServer>
#include <TSystemGlobal>
#include <TActionThread>
#include "tmultiplexingreceiver.h"
#include "thttpbuffer.h"
#include "tfcore_unix.h"

const int MAX_EVENTS = 1024;
static TMultiplexingReceiver *multiplexingReceiver = 0;

static void cleanup()
{
    if (multiplexingReceiver) {
        delete multiplexingReceiver;
        multiplexingReceiver = 0;
    }
}


void TMultiplexingReceiver::instantiate()
{
    if (!multiplexingReceiver) {
        multiplexingReceiver = new TMultiplexingReceiver();
        qAddPostRoutine(::cleanup);
    }

}


TMultiplexingReceiver *TMultiplexingReceiver::instance()
{
    if (!multiplexingReceiver) {
        tFatal("Call TMultiplexingReceiver::instantiate() function first");
    }
    return multiplexingReceiver;
}


static void setNonBlocking(int sock)
{
    int flag = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flag | O_NONBLOCK);
}


TMultiplexingReceiver::TMultiplexingReceiver()
    : QThread(Tf::app()), stopped(false), listenSocket(0), epollFd(0), sendRequest()
{ }


TMultiplexingReceiver::~TMultiplexingReceiver()
{
    if (listenSocket > 0)
        TF_CLOSE(listenSocket);

    if (epollFd > 0)
        TF_CLOSE(epollFd);
}


void TMultiplexingReceiver::epollClose(int fd)
{
    tf_epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, NULL);
    TF_CLOSE(fd);
    bufferings.remove(fd);
}


int TMultiplexingReceiver::epollAdd(int fd, int events)
{
    struct epoll_event ev;

    memset(&ev, 0, sizeof(ev));
    ev.events = events;
    ev.data.fd = fd;
    return tf_epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &ev);
}


int TMultiplexingReceiver::epollModify(int fd, int events)
{
    struct epoll_event ev;

    memset(&ev, 0, sizeof(ev));
    ev.events = events;
    ev.data.fd = fd;
    return tf_epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &ev);
}

void TMultiplexingReceiver::run()
{
    // Listen socket
    quint16 port = Tf::app()->appSettings().value("ListenPort").toUInt();
    int sock = TApplicationServer::nativeListen(QHostAddress::Any, port);
    if (sock > 0) {
        tSystemDebug("listen successfully.  port:%d", port);
    } else {
        tSystemError("Failed to set socket descriptor: %d", sock);
        TApplicationServer::nativeClose(sock);
        return;
    }
    listenSocket = sock;

    // Loads libs
    TApplicationServer::loadLibraries();

    // Create epoll
    epollFd = epoll_create(MAX_EVENTS);
    if (epollFd < 0) {
        tSystemError("Failed epoll_create()");
        goto socket_error;
    }

    if (epollAdd(listenSocket, EPOLLIN) < 0) {
        tSystemError("Failed epoll_ctl()");
        goto epoll_error;
    }

    struct epoll_event events[MAX_EVENTS];
    char buffer[BUFSIZ];

    for (;;) {
        // Poll Sending/Receiving/Incoming
        int nfd = tf_epoll_wait(epollFd, events, MAX_EVENTS, 10); // 10ms
        if (nfd < 0) {
            tSystemError("Failed epoll_wait() : errno:%d", errno);
            break;
        }

        for (int i = 0; i < nfd; ++i) {
            if (events[i].data.fd == listenSocket) {
                // Incoming connection
                struct sockaddr_in addr;
                socklen_t addrlen = sizeof(addr);
                int clt = ::accept(events[i].data.fd, (sockaddr *)&addr, &addrlen);
                if (clt < 0) {
                    tSystemWarn("Failed accept");
                    continue;
                }

                setNonBlocking(clt);
                if (epollAdd(clt, EPOLLIN) < 0) {
                    epollClose(clt);
                } else {
                    THttpBuffer &httpBuf = bufferings[clt];
                    httpBuf.clear();
                    httpBuf.setClientAddress(QHostAddress((sockaddr *)&addr));
                }

            } else {
                int cltfd = events[i].data.fd;

                if ( (events[i].events & EPOLLIN) ) {
                    // Receive data
                    int len = ::recv(cltfd, buffer, BUFSIZ, 0);

                    if (len > 0) {
                        // Read successfully
                        THttpBuffer &httpbuf = bufferings[cltfd];
                        httpbuf.write(buffer, len);

                        if (httpbuf.canReadHttpRequest()) {
                            incomingRequest(cltfd, httpbuf.read());
                            httpbuf.clear();

                            // Stop polling
                            if (tf_epoll_ctl(epollFd, EPOLL_CTL_DEL, cltfd, NULL)) {
                                epollClose(cltfd);
                            }
                        }

                    } else if (len == 0) {
                        // Disconnected
                        epollClose(cltfd);

                    } else {
                        tSystemError("Failed read : errno:%d", errno);
                        epollClose(cltfd);
                    }

                } else if ( (events[i].events & EPOLLOUT) ) {
                    // Send data
                    QByteArray &httpbuf = bufferings[cltfd].buffer();
                    int len = qMin(httpbuf.length(), BUFSIZ);
                    int sentlen = ::send(cltfd, httpbuf.constData(), len, 0);

                    if (sentlen <= 0) {
                        tSystemError("Failed send : errno:%d", errno);
                        epollClose(cltfd);

                    } else {
                        httpbuf.remove(0, sentlen);

                        if (httpbuf.length() == 0) {
                            // Prepare recv
                            if (epollModify(cltfd, EPOLLIN) < 0) {
                                epollClose(cltfd);
                            }
                        }
                    }
                } else {
                    // do nothing
                }
            }
        }

        // Check send-request
        QPair<int, QByteArray> *req = sendRequest.fetchAndStoreRelaxed(0);
        if (req) {
            int fd = req->first;
            bufferings[req->first].write(req->second);
            delete req;

            // Set epoll for sending
            if (epollAdd(fd, EPOLLOUT) < 0) {
                epollClose(fd);
            }
        }

        // Check stop flag
        if (stopped)
            break;
    }

epoll_error:
    TF_CLOSE(epollFd);
    epollFd = 0;
socket_error:
    TF_CLOSE(listenSocket);
    listenSocket = 0;
}


void TMultiplexingReceiver::incomingRequest(int fd, const THttpRequest &request)
{
    for (;;) {
//        if (actionContextCount() < maxServers) {
          TActionThread *thread = new TActionThread(fd, request);
//            connect(thread, SIGNAL(finished()), this, SLOT(deleteActionContext()));
//            insertPointer(thread);
            thread->start();
            break;
//        }
//        Tf::msleep(1);
//        qApp->processEvents(QEventLoop::ExcludeSocketNotifiers);
    }
}


qint64 TMultiplexingReceiver::setSendRequest(int fd, const QByteArray &buffer)
{
    if (fd <= 0)
        return -1;

    QPair<int, QByteArray> *pair = new QPair<int, QByteArray>(fd, buffer);
    bool ret = sendRequest.testAndSetRelaxed(0, pair);
    if (!ret) {
        delete pair;
        return 0;
    }
    return buffer.length();
}


qint64 TMultiplexingReceiver::setSendRequest(int fd, const THttpHeader *header, QIODevice *body)
{
    QByteArray sendbuf;

    if (body && !body->isOpen()) {
        if (!body->open(QIODevice::ReadOnly)) {
            tWarn("open failed");
            return -1;
        }
    }

    // Writes HTTP header
    QByteArray hdata = header->toByteArray();
    sendbuf += hdata;

    if (body) {
        QBuffer *buffer = qobject_cast<QBuffer *>(body);
        if (buffer) {
            sendbuf += buffer->data();
        } else {
            sendbuf += body->readAll();
        }
    }

    qint64 ret;
    while ((ret = setSendRequest(fd, sendbuf)) == 0) {
        Tf::msleep(1);
    }

    return ret;
}

#endif // Q_OS_LINUX
