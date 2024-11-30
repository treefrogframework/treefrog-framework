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


static void setNoDeleyOption(int fd)
{
    int res, flag, bufsize;

    // Disable the Nagle (TCP No Delay) algorithm
    flag = 1;
    res = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag));
    if (res < 0) {
        tSystemWarn("setsockopt error [TCP_NODELAY] fd:{}", fd);
    }

    bufsize = SEND_BUF_SIZE;
    res = setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));
    if (res < 0) {
        tSystemWarn("setsockopt error [SO_SNDBUF] fd:{}", fd);
    }

    bufsize = RECV_BUF_SIZE;
    res = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));
    if (res < 0) {
        tSystemWarn("setsockopt error [SO_RCVBUF] fd:{}", fd);
    }
}


TMultiplexingServer::TMultiplexingServer(int listeningSocket, QObject *parent) :
    TDatabaseContextThread(parent),
    TApplicationServerBase(),
    listenSocket(listeningSocket),
    reloadTimer()
{
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


int TMultiplexingServer::processEvents(int maxMilliSeconds)
{
    TEpoll::instance()->dispatchEvents();

    // Poll Sending/Receiving/Incoming
    maxMilliSeconds = std::max(maxMilliSeconds, 0);
    int res = TEpoll::instance()->wait(maxMilliSeconds);
    if (res < 0) {
        return res;
    }

    TEpollSocket *sock;
    while ((sock = TEpoll::instance()->next())) {

        int cltfd = sock->socketDescriptor();
        if (cltfd == listenSocket && cltfd > 0) {
            if (_processingSocketStack.count() > 64) {  // Not accept in deep context
                continue;
            }

            TEpollSocket *acceptedSock = TEpollHttpSocket::accept(listenSocket);
            if (Q_LIKELY(acceptedSock)) {
                if (!acceptedSock->watch()) {
                    acceptedSock->dispose();
                }
            }
            continue;

        } else {
            if (TEpoll::instance()->canSend()) {
                if (sock->state() == Tf::SocketState::Connecting) {
                    sock->_state = Tf::SocketState::Connected;
                    continue;
                }

                // Send data
                int len = TEpoll::instance()->send(sock);
                if (Q_UNLIKELY(len < 0)) {
                    TEpoll::instance()->deletePoll(sock);
                    sock->dispose();
                    continue;
                }
            }

            if (TEpoll::instance()->canReceive()) {
                try {
                    // Receive data
                    int len = TEpoll::instance()->recv(sock);
                    if (Q_UNLIKELY(len < 0)) {
                        TEpoll::instance()->deletePoll(sock);
                        sock->dispose();
                        continue;
                    }
                } catch (ClientErrorException &e) {
                    Tf::warn("Caught ClientErrorException: status code:{}", e.statusCode());
                    tSystemWarn("Caught ClientErrorException: status code:{}", e.statusCode());
                    TEpoll::instance()->deletePoll(sock);
                    sock->dispose();
                    continue;
                }

                if (sock->canReadRequest()) {
                    _processingSocketStack.push(sock);
                    sock->process();
                    _processingSocketStack.pop();
                }
            }
        }
    }

    // Garbage
    if (!_garbageSockets.isEmpty()) {
        auto set = _garbageSockets;
        for (auto ptr : set) {
            if (!ptr->isProcessing()) {
                delete ptr;  // Remove it from garbageSockets-set in the destructor
            }
        }
    }

    return res;
}


void TMultiplexingServer::run()
{
    setNoDeleyOption(listenSocket);

    TEpollSocket *epollListen = TEpollHttpSocket::create(listenSocket, QHostAddress(), false);
    TEpoll::instance()->addPoll(epollListen, EPOLLIN);

    int keepAlivetimeout = Tf::appSettings()->value(Tf::HttpKeepAliveTimeout).toInt();
    QElapsedTimer idleTimer;
    if (keepAlivetimeout > 0) {
        idleTimer.start();
    }

    QElapsedTimer elapsed;
    elapsed.start();

    for (;;) {
        int res = processEvents(100);
        if (res < 0) {
            break;
        }

        // Check keep-alive timeout for HTTP sockets
        if (keepAlivetimeout > 0 && idleTimer.elapsed() >= 1000) {
            for (auto *http : (const QList<TEpollHttpSocket *> &)TEpollHttpSocket::allSockets()) {
                if (Q_UNLIKELY(http->socketDescriptor() != listenSocket && http->idleTime() >= keepAlivetimeout)) {
                    tSystemDebug("KeepAlive timeout: socket:{}", http->socketDescriptor());
                    TEpoll::instance()->deletePoll(http);
                    http->dispose();
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


TActionWorker *TMultiplexingServer::currentWorker() const
{
    if (!_processingSocketStack.isEmpty()) {
        TEpollSocket *socket = _processingSocketStack.top();
        if (socket) {
            auto ptr = dynamic_cast<TEpollHttpSocket *>(socket);
            if (ptr) {
                return ptr->worker();
            }
        }
    }
    return nullptr;
}


TActionController *TMultiplexingServer::currentController() const
{
    auto *worker = currentWorker();
    return (worker) ? worker->currentController() : nullptr;
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
