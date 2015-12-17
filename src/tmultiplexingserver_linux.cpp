/* Copyright (c) 2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <netinet/tcp.h>
#include <QElapsedTimer>
#include <TWebApplication>
#include <TAppSettings>
#include <TApplicationServerBase>
#include <TMultiplexingServer>
#include <TThreadApplicationServer>
#include <TSystemGlobal>
#include <TActionWorker>
#include "tepoll.h"
#include "tepollsocket.h"
#include "tepollhttpsocket.h"

const int SEND_BUF_SIZE = 16 * 1024;
const int RECV_BUF_SIZE = 128 * 1024;
static TMultiplexingServer *multiplexingServer = 0;


static void cleanup()
{
    if (multiplexingServer) {
        delete multiplexingServer;
        multiplexingServer = 0;
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


TMultiplexingServer::TMultiplexingServer(int listeningSocket, QObject *parent)
    : QThread(parent), TApplicationServerBase(), maxWorkers(0), stopped(false),
      listenSocket(listeningSocket), reloadTimer()
{
    Q_ASSERT(Tf::app()->multiProcessingModule() == TWebApplication::Hybrid);
}


TMultiplexingServer::~TMultiplexingServer()
{ }


bool TMultiplexingServer::start()
{
    if (isRunning())
        return true;

    // Loads libs
    TApplicationServerBase::loadLibraries();

    TStaticInitializeThread::exec();
    QThread::start();
    return true;
}


void TMultiplexingServer::run()
{
    QString mpm = Tf::appSettings()->value(Tf::MultiProcessingModule).toString().toLower();
    maxWorkers = Tf::appSettings()->readValue(QLatin1String("MPM.") + mpm + ".MaxWorkersPerAppServer").toInt();
    if (maxWorkers <= 0) {
        maxWorkers = Tf::appSettings()->readValue(QLatin1String("MPM.") + mpm + ".MaxWorkersPerServer", "128").toInt();
    }
    tSystemDebug("MaxWorkers: %d", maxWorkers);

    int appsvrnum = qMax(Tf::app()->maxNumberOfAppServers(), 1);
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
        if (!numEvents && TActionWorker::workerCount() > 0) {
            TEpoll::instance()->waitSendData(4);  // mitigation of busy loop
        }

        TEpoll::instance()->dispatchSendData();

        // Poll Sending/Receiving/Incoming
        int timeout = (TActionWorker::workerCount() > 0) ? 0 : 100;
        numEvents = TEpoll::instance()->wait(timeout);
        if (numEvents < 0)
            break;

        TEpollSocket *sock;
        while ( (sock = TEpoll::instance()->next()) ) {

            int cltfd = sock->socketDescriptor();
            if (cltfd == listenSocket) {
                for (;;) {
                    TEpollSocket *acceptedSock = TEpollSocket::accept(listenSocket);
                    if (Q_UNLIKELY(!acceptedSock))
                        break;

                    TEpoll::instance()->addPoll(acceptedSock, (EPOLLIN | EPOLLOUT | EPOLLET));

                    if (appsvrnum > 1) {
                        break;  // Load smoothing
                    }
                }
                continue;

            } else {
                if ( TEpoll::instance()->canSend() ) {
                    // Send data
                    int len = TEpoll::instance()->send(sock);
                    if (Q_UNLIKELY(len < 0)) {
                        TEpoll::instance()->deletePoll(sock);
                        sock->close();
                        sock->deleteLater();
                        continue;
                    }
                }

                if ( TEpoll::instance()->canReceive() ) {
                    if (TActionWorker::workerCount() >= maxWorkers) {
                        // not receive
                        TEpoll::instance()->modifyPoll(sock, (EPOLLIN | EPOLLOUT | EPOLLET));  // reset
                        continue;
                    }

                    if (sock->countWorker() > 0) {
                        // not receive
                        sock->pollIn = true;
                        continue;
                    }

                    // Receive data
                    int len = TEpoll::instance()->recv(sock);
                    if (Q_UNLIKELY(len < 0)) {
                        TEpoll::instance()->deletePoll(sock);
                        sock->close();
                        sock->deleteLater();
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
            idleTimer.start();

            auto sockets = TEpollHttpSocket::allSockets();
            for (auto *http : sockets) {
                if (Q_UNLIKELY(http->socketDescriptor() != listenSocket && http->idleTime() >= keepAlivetimeout)) {
                    tSystemDebug("KeepAlive timeout: %s", http->socketUuid().data());
                    TEpoll::instance()->deletePoll(http);
                    http->close();
                    http->deleteLater();
                }
            }
        }

        // Check stop flag
        if (stopped.load()) {
            break;
        }
    }

    TEpoll::instance()->releaseAllPollingSockets();
    TActionWorker::waitForAllDone(10000);
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
