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
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <QHostAddress>
#include <QBuffer>
#include <TWebApplication>
#include <TApplicationServerBase>
#include <TMultiplexingServer>
#include <TThreadApplicationServer>
#include <TSystemGlobal>
#include <TActionWorker>
#include <THttpHeader>
#include <THttpRequest>
#include "thttpbuffer.h"
#include "thttpsendbuffer.h"
#include "tfcore_unix.h"

const int SEND_BUF_SIZE = 256 * 1024;
const int RECV_BUF_SIZE = 256 * 1024;
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
        TWorkerStarter *starter = new TWorkerStarter(multiplexingServer);
        connect(multiplexingServer, SIGNAL(incomingHttpRequest(int, const QByteArray &, const QString &)), starter, SLOT(startWorker(int, const QByteArray &, const QString &)));
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


// static void setNonBlocking(int sock)
// {
//     int flag = fcntl(sock, F_GETFL);
//     fcntl(sock, F_SETFL, flag | O_NONBLOCK);
// }


static void setSocketOption(int fd)
{
    int ret, flag, bufsize;

    // Disable the Nagle (TCP No Delay) algorithm
    flag = 1;
    ret = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag));
    if (ret < 0) {
        tSystemWarn("setsockopt error [TCP_NODELAY] fd:%d", fd);
    }

    bufsize = SEND_BUF_SIZE;
    ret = setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));
    if (ret < 0) {
        tSystemWarn("setsockopt error [SO_SNDBUF] fd:%d", fd);
    }

    bufsize = RECV_BUF_SIZE;
    ret = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));
    if (ret < 0) {
        tSystemWarn("setsockopt error [SO_RCVBUF] fd:%d", fd);
    }
}


TMultiplexingServer::TMultiplexingServer(QObject *parent)
    : QThread(parent), TApplicationServerBase(), maxWorkers(0), stopped(false),
      listenSocket(0), epollFd(0), sendRequests(), threadCounter(0)
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
    QQueue<THttpSendBuffer*> que = sendBuffers.take(fd);
    for (QListIterator<THttpSendBuffer*> it(que); it.hasNext(); ) {
        delete it.next();
    }
    pendingRequests.removeAll(fd);
}


int TMultiplexingServer::epollAdd(int fd, int events)
{
    struct epoll_event ev;

    memset(&ev, 0, sizeof(ev));
    ev.events = events;
    ev.data.fd = fd;

    int ret = tf_epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &ev);
    int err = errno;
    if (ret < 0 && err != EEXIST) {
        tSystemError("Failed epoll_ctl (EPOLL_CTL_ADD)  fd:%d errno:%d", fd, errno);
        epollClose(fd);
    } else {
        //tSystemDebug("OK epoll_ctl (EPOLL_CTL_ADD) (events:%d)  fd:%d", events, fd);
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
    } else {
        tSystemDebug("OK epoll_ctl (EPOLL_CTL_MOD)  fd:%d", fd);
    }
    return ret;
}


int TMultiplexingServer::epollDel(int fd)
{
    int ret = tf_epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, NULL);
    int err = errno;
    if (ret < 0 && err != ENOENT) {
        tSystemError("Failed epoll_ctl (EPOLL_CTL_DEL)  fd:%d errno:%d", fd, errno);
        epollClose(fd);
    } else {
        tSystemDebug("OK epoll_ctl (EPOLL_CTL_DEL)  fd:%d", fd);
    }
    return ret;
}


int TMultiplexingServer::getSendRequest()
{
    // Check send-request
    QList<SendData*> reqQueue = sendRequests.dequeue();

    for (QListIterator<SendData*> it(reqQueue); it.hasNext(); ) {
        SendData *req = it.next();
        int fd = req->fd;
        if (req->method == SendData::Send) {
            if (recvBuffers.contains(fd)) {
                // Add to a send-buffer
                sendBuffers[fd].enqueue(req->buffer);
                // Set epoll for sending and recieving
                epollModify(fd, EPOLLIN | EPOLLOUT);
            } else {
                delete req->buffer;
            }

        } else if (req->method == SendData::Disconnect) {
            if (recvBuffers.contains(fd) || sendBuffers.contains(fd)) {
                epollClose(fd);
            }
        } else {
            Q_ASSERT(0);
        }
        delete req;
    }

    return reqQueue.count();
}


void TMultiplexingServer::emitIncomingRequest(int fd, THttpBuffer &buffer)
{
    QHostAddress host = buffer.clientAddress();
    emit incomingHttpRequest(fd, buffer.read(INT_MAX), host.toString());
    threadCounter.fetchAndAddOrdered(1);
    buffer.clear();
    buffer.setClientAddress(host); // inherits the host adress
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
    setSocketOption(listenSocket);

    maxWorkers = Tf::app()->maxNumberOfServers(10);
    tSystemDebug("MaxWorkers: %d", maxWorkers);

    // Get send buffer size and recv buffer size
    int res, sendBufSize, recvBufSize;
    socklen_t optlen = sizeof(int);
    res = getsockopt(listenSocket, SOL_SOCKET, SO_SNDBUF, &sendBufSize, &optlen);
    if (res < 0)
        tSystemDebug("SO_SNDBUF: %d", sendBufSize);

    res = getsockopt(listenSocket, SOL_SOCKET, SO_RCVBUF, &recvBufSize, &optlen);
    if (res < 0)
        tSystemDebug("SO_RCVBUF: %d", recvBufSize);

    const int MaxEvents = 128;
    struct epoll_event events[MaxEvents];
    sendBufSize *= 0.8;
    char *sndbuffer = new char[sendBufSize];
    char *rcvbuffer = new char[recvBufSize];
    int nfd = 0;

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
        // Check pending requests
        while (!pendingRequests.isEmpty() && countWorkers() < maxWorkers) {
            int fd = pendingRequests.takeFirst();
            THttpBuffer &recvbuf = recvBuffers[fd];
            emitIncomingRequest(fd, recvbuf);
        }

        if (!nfd && countWorkers() > 0) {
            sendRequests.wait(5);  // mitigation of busy loop
        }

        // Get send-request
        getSendRequest();

        // Poll Sending/Receiving/Incoming
        int timeout = (countWorkers() > 0) ? 0 : 100;
        nfd = tf_epoll_wait(epollFd, events, MaxEvents, timeout);
        int err = errno;
        if (nfd < 0) {
            tSystemError("Failed epoll_wait() : errno:%d", err);
            break;
        }

        for (int i = 0; i < nfd; ++i) {
            if (events[i].data.fd == listenSocket) {
                if (!pendingRequests.isEmpty())
                    continue;

                // Incoming connection
                struct sockaddr_storage addr;
                socklen_t addrlen = sizeof(addr);

                int clt = ::accept4(events[i].data.fd, (sockaddr *)&addr, &addrlen, SOCK_CLOEXEC | SOCK_NONBLOCK);
                if (clt < 0) {
                    tSystemWarn("Failed accept");
                    continue;
                }
                setSocketOption(clt);

                if (epollAdd(clt, EPOLLIN) == 0) {
                    THttpBuffer &recvbuf = recvBuffers[clt];
                    recvbuf.clear();
                    recvbuf.setClientAddress(QHostAddress((sockaddr *)&addr));
                }

            } else {
                int cltfd = events[i].data.fd;

                if ( (events[i].events & EPOLLIN) ) {
                    // Receive data
                    int len = ::recv(cltfd, rcvbuffer, recvBufSize, 0);
                    err = errno;
                    if (len > 0) {
                        // Read successfully
                        THttpBuffer &recvbuf = recvBuffers[cltfd];
                        recvbuf.write(rcvbuffer, len);

                        if (recvbuf.canReadHttpRequest()) {
                            // Incoming a request
                            if (countWorkers() >= maxWorkers) {
                                pendingRequests << cltfd;
                            } else {
                                emitIncomingRequest(cltfd, recvbuf);
                            }
                        }

                    } else {
                        if (len < 0 && err != ECONNRESET) {
                            tSystemError("Failed recv : errno:%d", err);
                        }

                        // Disconnect
                        epollClose(cltfd);
                        continue;
                    }
                }

                if ( (events[i].events & EPOLLOUT) ) {
                    // Send data
                    THttpSendBuffer *sendbuf = sendBuffers[cltfd].first();
                    if (!sendbuf) {
                        tSystemError("Not found send-buffer");
                        epollClose(cltfd);
                        continue;
                    }

                    int len = sendbuf->read(sndbuffer, sendBufSize);
                    int sentlen = ::send(cltfd, sndbuffer, len, 0);
                    err = errno;
                    TAccessLogger &logger = sendbuf->accessLogger();

                    if (sentlen <= 0) {
                        if (err != ECONNRESET) {
                            tSystemError("Failed send : errno:%d", err);
                        }
                        // Access log
                        logger.setResponseBytes(-1);
                        logger.write();

                        epollClose(cltfd);
                        continue;
                    } else {
                        logger.setResponseBytes(logger.responseBytes() + sentlen);

                        if (len > sentlen) {
                            tSystemDebug("sendbuf prepend: len:%d", len - sentlen);
                            sendbuf->prepend(sndbuffer + sentlen, len - sentlen);
                        }

                        if (sendbuf->atEnd()) {
                            logger.write();  // Writes access log
                            sendbuf->release();

                            QQueue<THttpSendBuffer*> &que = sendBuffers[cltfd];
                            delete que.dequeue(); // delete send-buffer obj

                            // Prepare recv
                            if (que.isEmpty())
                                epollModify(cltfd, EPOLLIN);
                        }
                    }
                }
            }
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
    }

epoll_error:
    TF_CLOSE(epollFd);
    epollFd = 0;

socket_error:
    if (listenSocket > 0)
        TF_CLOSE(listenSocket);
    listenSocket = 0;
    delete sndbuffer;
    delete rcvbuffer;
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

    sendRequests.enqueue(sd);
}


void TMultiplexingServer::setDisconnectRequest(int fd)
{
    if (fd <= 0)
        return;

    SendData *sd = new SendData;
    sd->method = SendData::Disconnect;
    sd->fd = fd;
    sd->buffer = 0;

    sendRequests.enqueue(sd);
}


void TMultiplexingServer::terminate()
{
    stopped = true;
    wait(10000);
}


void TMultiplexingServer::deleteActionContext()
{
    TActionWorker *worker = qobject_cast<TActionWorker *>(sender());
    Q_CHECK_PTR(worker);
    deletePointer(worker);
    worker->deleteLater();
    threadCounter.fetchAndAddOrdered(-1);
}


/*
 * TWorkerStarter class
 */

TWorkerStarter::~TWorkerStarter()
{ }


void TWorkerStarter::startWorker(int fd, const QByteArray &request, const QString &address)
{
    //
    // Create worker threads in main thread for signal/slot mechanism!
    //
    TActionWorker *worker = new TActionWorker(fd, THttpRequest(request, QHostAddress(address)));
    connect(worker, SIGNAL(finished()), multiplexingServer, SLOT(deleteActionContext()));
    multiplexingServer->insertPointer(worker);
    worker->start();
}
