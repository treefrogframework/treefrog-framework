/* Copyright (c) 2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

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
#include <TApplicationServerBase>
#include <TMultiplexingServer>
#include <TThreadApplicationServer>
#include <TSystemGlobal>
#include <TActionWorker>
#include "thttpbuffer.h"
#include "thttpsendbuffer.h"
#include "tfcore_unix.h"

static TMultiplexingServer *multiplexingServer = 0;


static void cleanup()
{
    if (multiplexingServer) {
        delete multiplexingServer;
        multiplexingServer = 0;
    }
}


void TMultiplexingServer::instantiate()
{
    if (!multiplexingServer) {
        multiplexingServer = new TMultiplexingServer();
        qAddPostRoutine(::cleanup);
    }
}


TMultiplexingServer *TMultiplexingServer::instance()
{
    if (!multiplexingServer) {
        tFatal("Call TMultiplexingServer::instantiate() function first");
    }
    return multiplexingServer;
}


static void setNonBlocking(int sock)
{
    int flag = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flag | O_NONBLOCK);
}


TMultiplexingServer::TMultiplexingServer()
    : QThread(), TApplicationServerBase(), maxWorkers(0), stopped(false), listenSocket(0), epollFd(0), sendRequest()
{
    connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(terminate()));
    Q_ASSERT(Tf::app()->multiProcessingModule() == TWebApplication::Hybrid);
}


TMultiplexingServer::~TMultiplexingServer()
{
    if (listenSocket > 0)
        TF_CLOSE(listenSocket);

    if (epollFd > 0)
        TF_CLOSE(epollFd);
}


bool TMultiplexingServer::start()
{
    if (isRunning())
        return true;

    // Loads libs
    TApplicationServerBase::loadLibraries();

    TStaticInitializeThread *initializer = new TStaticInitializeThread();
    initializer->start();
    initializer->wait();
    delete initializer;

    connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
    QThread::start();
    return true;
}


void TMultiplexingServer::epollClose(int fd)
{
    tf_epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, NULL);
    TF_CLOSE(fd);
    recvBuffers.remove(fd);
    THttpSendBuffer *p = sendBuffers.take(fd);
    if (p) {
        delete p;
    }
}


int TMultiplexingServer::epollAdd(int fd, int events)
{
    struct epoll_event ev;

    memset(&ev, 0, sizeof(ev));
    ev.events = events;
    ev.data.fd = fd;

    int ret = tf_epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &ev);
    if (ret < 0) {
        tSystemError("Failed epoll_ctl (EPOLL_CTL_ADD)  fd:%d errno:%d", fd, errno);
        epollClose(fd);
    }
    return ret;
}


int TMultiplexingServer::epollModify(int fd, int events)
{
    struct epoll_event ev;

    memset(&ev, 0, sizeof(ev));
    ev.events = events;
    ev.data.fd = fd;

    int ret = tf_epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &ev);
    if (ret < 0) {
        tSystemError("Failed epoll_ctl (EPOLL_CTL_MOD)  fd:%d errno:%d", fd, errno);
        epollClose(fd);
    }
    return ret;
}


int TMultiplexingServer::epollDel(int fd)
{
    int ret = tf_epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, NULL);
    if (ret < 0) {
        tSystemError("Failed epoll_ctl (EPOLL_CTL_DEL)  fd:%d errno:%d", fd, errno);
        epollClose(fd);
    }
    return ret;
}


void TMultiplexingServer::checkSendRequest(int &actionCount)
{
    // Check send-request
    SendData *req = sendRequest.fetchAndStoreRelaxed(0);
    if (req) {
        int fd = req->fd;
        if (req->method == SendData::Send) {
            Q_ASSERT(sendBuffers[fd] == NULL);
            // Add to a send-buffer
            sendBuffers[fd] = req->buffer;
            // Set epoll for sending
            epollAdd(fd, EPOLLOUT);
        } else if (req->method == SendData::Disconnect) {
            epollClose(fd);
        } else {
            Q_ASSERT(0);
        }

        delete req;
        --actionCount;
    }
}


void TMultiplexingServer::run()
{
    // Listen socket
    quint16 port = Tf::app()->appSettings().value("ListenPort").toUInt();
    int sock = TApplicationServerBase::nativeListen(QHostAddress::Any, port);
    if (sock > 0) {
        tSystemDebug("listen successfully.  port:%d", port);
    } else {
        tSystemError("Failed to set socket descriptor: %d", sock);
        TApplicationServerBase::nativeClose(sock);
        return;
    }
    listenSocket = sock;

    maxWorkers = Tf::app()->maxNumberOfServers(10);
    tSystemDebug("MaxWorkers: %d", maxWorkers);

    const int MaxEvents = 16;
    struct epoll_event events[MaxEvents];
    char buffer[BUFSIZ];
    int actionCount = 0;

    // Create epoll
    epollFd = epoll_create(1);
    if (epollFd < 0) {
        tSystemError("Failed epoll_create()");
        goto socket_error;
    }

    if (epollAdd(listenSocket, EPOLLIN) < 0) {
        tSystemError("Failed epoll_ctl()");
        goto epoll_error;
    }

    for (;;) {
        // Poll Sending/Receiving/Incoming
        int nfd = tf_epoll_wait(epollFd, events, MaxEvents, (actionCount > 0 ? 1 : 100));
        if (nfd < 0) {
            tSystemError("Failed epoll_wait() : errno:%d", errno);
            break;
        }

        for (int i = 0; i < nfd; ++i) {
            if (events[i].data.fd == listenSocket) {
                // Incoming connection
                struct sockaddr_storage addr;
                socklen_t addrlen = sizeof(addr);

                int clt = ::accept(events[i].data.fd, (sockaddr *)&addr, &addrlen);
                if (clt < 0) {
                    tSystemWarn("Failed accept");
                    continue;
                }

                setNonBlocking(clt);
                if (epollAdd(clt, EPOLLIN) == 0) {
                    THttpBuffer &recvbuf = recvBuffers[clt];
                    recvbuf.clear();
                    recvbuf.setClientAddress(QHostAddress((sockaddr *)&addr));
                }

            } else {
                int cltfd = events[i].data.fd;

                if ( (events[i].events & EPOLLIN) ) {
                    // Receive data
                    int len = ::recv(cltfd, buffer, BUFSIZ, 0);

                    if (len > 0) {
                        // Read successfully
                        THttpBuffer &recvbuf = recvBuffers[cltfd];
                        recvbuf.write(buffer, len);

                        if (recvbuf.canReadHttpRequest()) {
                            if (incomingRequest(cltfd, recvbuf.read())) {
                                QHostAddress host = recvbuf.clientAddress();
                                recvbuf.clear();
                                recvbuf.setClientAddress(host); // inherits the host adress
                                ++actionCount;
                                epollDel(cltfd);  // Stop polling
                            } else {
                                epollClose(cltfd);
                            }
                        }

                    } else {
                        if (len < 0) {
                            tSystemError("Failed read : errno:%d", errno);
                        }

                        // Disconnect
                        epollClose(cltfd);

                        if (recvBuffers.isEmpty() && sendBuffers.isEmpty()) {
                            actionCount = 0;
                        }
                    }

                } else if ( (events[i].events & EPOLLOUT) ) {
                    // Send data
                    THttpSendBuffer *sendbuf = sendBuffers[cltfd];
                    if (!sendbuf) {
                        tSystemError("Not found send-buffer");
                        epollClose(cltfd);
                        continue;
                    }

                    int len = sendbuf->read(buffer, BUFSIZ);
                    int sentlen = ::send(cltfd, buffer, len, 0);
                    TAccessLogger &logger = sendbuf->accessLogger();

                    if (sentlen <= 0) {
                        tSystemError("Failed send : errno:%d", errno);
                        // Access log
                        logger.setResponseBytes(-1);
                        logger.write();

                        epollClose(cltfd);
                    } else {
                        logger.setResponseBytes(logger.responseBytes() + sentlen);

                        if (sendbuf->atEnd()) {
                            logger.write();  // Writes access log
                            sendbuf->release();
                            delete sendBuffers.take(cltfd); // delete send-buffer obj

                            // Prepare recv
                            epollModify(cltfd, EPOLLIN);
                        }
                    }
                } else {
                    // do nothing
                }
            }

            // Check send-request
            checkSendRequest(actionCount);
        }

        // Check stop flag
        if (stopped) {
            if (listenSocket > 0) {
                // Close the listen-socket
                epollClose(listenSocket);
                listenSocket = 0;
            }

            if (!recvBuffers.isEmpty()) {
                for (QMapIterator<int, THttpBuffer> it(recvBuffers); it.hasNext(); ) {
                    it.next();
                    epollClose(it.key());
                }
            }

            if (recvBuffers.isEmpty() && sendBuffers.isEmpty()) {
                break;
            }
        }

        // Check send-request
        checkSendRequest(actionCount);
    }

epoll_error:
    TF_CLOSE(epollFd);
    epollFd = 0;

socket_error:
    if (listenSocket > 0)
        TF_CLOSE(listenSocket);
    listenSocket = 0;
}


bool TMultiplexingServer::incomingRequest(int fd, const THttpRequest &request)
{
    int cnt = actionContextCount();
    if (cnt >= maxWorkers) {
        tSystemWarn("No more action thread to start [count:%d]. Adjust the value of the MPM.hybrid.MaxServers parameter.", cnt);
        return false;
    }

    TActionWorker *thread = new TActionWorker(fd, request);
    connect(thread, SIGNAL(finished()), this, SLOT(deleteActionContext()));
    insertPointer(thread);
    thread->start();
    return true;
}


void TMultiplexingServer::setSendRequest(int fd, const THttpHeader *header, QIODevice *body, bool autoRemove, const TAccessLogger &accessLogger)
{
    SendData *sd = new SendData;
    sd->method = SendData::Send;
    sd->fd = fd;
    sd->buffer = 0;

    QByteArray response = header->toByteArray();
    QFileInfo fi;

    if (body) {
        QBuffer *buffer = qobject_cast<QBuffer *>(body);
        if (buffer) {
            response += buffer->data();
        } else {
            fi.setFile(*qobject_cast<QFile *>(body));
        }
    }
    sd->buffer = new THttpSendBuffer(response, fi, autoRemove, accessLogger);

    while (!sendRequest.testAndSetRelaxed(0, sd)) {
        if (stopped) {
            delete sd->buffer;
            delete sd;
            break;
        }
        Tf::msleep(1);
    }
}


void TMultiplexingServer::setDisconnectRequest(int fd)
{
    if (fd <= 0)
        return;

    SendData *sd = new SendData;
    sd->method = SendData::Disconnect;
    sd->fd = fd;
    sd->buffer = 0;

    while (!sendRequest.testAndSetRelaxed(0, sd)) {
        if (stopped) {
            delete sd;
            break;
        }
        Tf::msleep(1);
    }
}


void TMultiplexingServer::terminate()
{
    stopped = true;
    wait(10000);
}


void TMultiplexingServer::deleteActionContext()
{
    deletePointer(reinterpret_cast<TActionThread *>(sender()));
    sender()->deleteLater();
}
