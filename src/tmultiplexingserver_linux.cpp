/* Copyright (c) 2015-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tepoll.h"
#include "tepollhttpsocket.h"
#include "tepollsocket.h"
#include "tkvsdatabasepool.h"
#include "tpublisher.h"
#include "tsqldatabasepool.h"
#include "tsystembus.h"
#include "tsystemglobal.h"
#include "turlroute.h"
#include <QElapsedTimer>
#include <TActionWorker>
#include <TAppSettings>
#include <TApplicationServerBase>
#include <TMultiplexingServer>
#include <TThreadApplicationServer>
#include <TWebApplication>
#include <netinet/tcp.h>

constexpr int SEND_BUF_SIZE = 16 * 1024;
constexpr int RECV_BUF_SIZE = 128 * 1024;

namespace {
TMultiplexingServer *multiplexingServer = nullptr;


void cleanup()
{
    delete multiplexingServer;
    multiplexingServer = nullptr;
}
}


void TMultiplexingServer::instantiate(int listeningSocket)
{
    if (!multiplexingServer) {
        multiplexingServer = new TMultiplexingServer(listeningSocket);
        qAddPostRoutine(::cleanup);
    }
}


TMultiplexingServer *TMultiplexingServer::instance()
{
    if (Q_UNLIKELY(!multiplexingServer)) {
        tFatal("Call TMultiplexingServer::instantiate() function first");
    }
    return multiplexingServer;
}


// static void setNonBlocking(int sock)
// {
//     int flag = fcntl(sock, F_GETFL);
//     fcntl(sock, F_SETFL, flag | O_NONBLOCK);
// }


static void setNoDeleyOption(int fd)
{
    int res, flag, bufsize;

    // Disable the Nagle (TCP No Delay) algorithm
    flag = 1;
    res = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag));
    if (res < 0) {
        tSystemWarn("setsockopt error [TCP_NODELAY] fd:%d", fd);
    }

    bufsize = SEND_BUF_SIZE;
    res = setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));
    if (res < 0) {
        tSystemWarn("setsockopt error [SO_SNDBUF] fd:%d", fd);
    }

    bufsize = RECV_BUF_SIZE;
    res = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));
    if (res < 0) {
        tSystemWarn("setsockopt error [SO_RCVBUF] fd:%d", fd);
    }
}


TMultiplexingServer::TMultiplexingServer(int listeningSocket, QObject *parent) :
    TDatabaseContextThread(parent),
    TApplicationServerBase(),
    listenSocket(listeningSocket),
    reloadTimer()
{
    Q_ASSERT(Tf::app()->multiProcessingModule() == TWebApplication::Epoll);
}


TMultiplexingServer::~TMultiplexingServer()
{
}


bool TMultiplexingServer::start(bool debugMode)
{
    if (isRunning())
        return true;

    // Loads libs
    bool res = loadLibraries();
    if (!res) {
        if (debugMode) {
            tSystemError("Failed to load application libraries.");
            return false;
        } else {
            tSystemWarn("Failed to load application libraries.");
        }
    }

    // To work a timer in main thread
    TSqlDatabasePool::instance();
    TKvsDatabasePool::instance();

    TStaticInitializeThread::exec();
    QThread::start();
    return true;
}


void TMultiplexingServer::run()
{
    setNoDeleyOption(listenSocket);

    TEpollSocket *lsn = TEpollSocket::create(listenSocket, QHostAddress());
    TEpoll::instance()->addPoll(lsn, EPOLLIN);
    int numEvents = 0;

    int keepAlivetimeout = Tf::appSettings()->value(Tf::HttpKeepAliveTimeout, "10").toInt();
    QElapsedTimer idleTimer;
    if (keepAlivetimeout > 0) {
        idleTimer.start();
    }

    for (;;) {
        TEpoll::instance()->dispatchSendData();

        // Poll Sending/Receiving/Incoming
        numEvents = TEpoll::instance()->wait(100);
        if (numEvents < 0) {
            break;
        }

        TEpollSocket *sock;
        while ((sock = TEpoll::instance()->next())) {

            int cltfd = sock->socketDescriptor();
            if (cltfd == listenSocket) {
                TEpollSocket *acceptedSock = TEpollSocket::accept(listenSocket);
                if (Q_LIKELY(acceptedSock)) {
                    if (!TEpoll::instance()->addPoll(acceptedSock, (EPOLLIN | EPOLLOUT | EPOLLET))) {
                        delete acceptedSock;
                    }
                }
                continue;

            } else {
                if (TEpoll::instance()->canSend()) {
                    // Send data
                    int len = TEpoll::instance()->send(sock);
                    if (Q_UNLIKELY(len < 0)) {
                        TEpoll::instance()->deletePoll(sock);
                        sock->close();
                        delete sock;
                        continue;
                    }
                }

                if (TEpoll::instance()->canReceive()) {
                    try {
                        // Receive data
                        int len = TEpoll::instance()->recv(sock);
                        if (Q_UNLIKELY(len < 0)) {
                            TEpoll::instance()->deletePoll(sock);
                            sock->close();
                            delete sock;
                            continue;
                        }
                    } catch (ClientErrorException &e) {
                        tWarn("Caught ClientErrorException: status code:%d", e.statusCode());
                        tSystemWarn("Caught ClientErrorException: status code:%d", e.statusCode());
                        TEpoll::instance()->deletePoll(sock);
                        sock->close();
                        delete sock;
                        continue;
                    }

                    if (sock->canReadRequest()) {
                        sock->startWorker();
                    }
                }
            }
        }

        // Check keep-alive timeout for HTTP sockets
        if (Q_UNLIKELY(keepAlivetimeout > 0 && idleTimer.elapsed() >= 1000)) {
            for (auto *http : (const QList<TEpollHttpSocket *> &)TEpollHttpSocket::allSockets()) {
                if (Q_UNLIKELY(http->socketDescriptor() != listenSocket && http->idleTime() >= keepAlivetimeout)) {
                    tSystemDebug("KeepAlive timeout: sid:%d", http->socketId());
                    TEpoll::instance()->deletePoll(http);
                    http->close();
                    delete http;
                }
            }
            idleTimer.start();
        }

        // Check stop flag
        if (stopped.load()) {
            break;
        }
    }

    TEpoll::instance()->releaseAllPollingSockets();
}


void TMultiplexingServer::stop()
{
    if (!stopped.exchange(true)) {
        if (isRunning()) {
            QThread::wait(10000);
        }
        TStaticReleaseThread::exec();
    }
}


void TMultiplexingServer::setAutoReloadingEnabled(bool enable)
{
    if (enable) {
        reloadTimer.start(500, this);
    } else {
        reloadTimer.stop();
    }
}


bool TMultiplexingServer::isAutoReloadingEnabled()
{
    return reloadTimer.isActive();
}


void TMultiplexingServer::timerEvent(QTimerEvent *event)
{
    if (event->timerId() != reloadTimer.timerId()) {
        QThread::timerEvent(event);
    } else {
        if (newerLibraryExists()) {
            tSystemInfo("Detect new library of application. Reloading the libraries.");
            Tf::app()->exit(127);
        }
    }
}
