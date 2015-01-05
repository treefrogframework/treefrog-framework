/* Copyright (c) 2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <netinet/tcp.h>
#include <TWebApplication>
#include <TAppSettings>
#include <TApplicationServerBase>
#include <TMultiplexingServer>
#include <TThreadApplicationServer>
#include <TSystemGlobal>
#include <TActionWorker>
#include "tepoll.h"
#include "tepollsocket.h"

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
        TWorkerStarter *starter = new TWorkerStarter(multiplexingServer);
        connect(multiplexingServer, SIGNAL(incomingRequest(TEpollSocket *)), starter, SLOT(startWorker(TEpollSocket *)), Qt::BlockingQueuedConnection);  // the emitter and receiver are in different threads.
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
      listenSocket(listeningSocket)
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

    TEpollSocket *sock = TEpollSocket::create(listenSocket, QHostAddress());
    TEpoll::instance()->addPoll(sock, EPOLLIN);
    int numEvents = 0;

    for (;;) {
        if (!numEvents && TActionWorker::workerCount() > 0) {
            TEpollSocket::waitSendData(5);  // mitigation of busy loop
        }

        TEpollSocket::dispatchSendData();

        // Poll Sending/Receiving/Incoming
        int timeout = (TActionWorker::workerCount() > 0) ? 0 : 100;
        numEvents = TEpoll::instance()->wait(timeout);
        if (numEvents < 0)
            break;

        TEpollSocket *epSock;
        while ( (epSock = TEpoll::instance()->next()) ) {

            int cltfd = epSock->socketDescriptor();
            if (cltfd == listenSocket) {
                for (;;) {
                    TEpollSocket *sock = TEpollSocket::accept(listenSocket);
                    if (Q_UNLIKELY(!sock))
                        break;

                    TEpoll::instance()->addPoll(sock, (EPOLLIN | EPOLLOUT | EPOLLET));

                    if (appsvrnum > 1) {
                        break;  // Load smoothing
                    }
                }
                continue;

            } else {
                if ( TEpoll::instance()->canSend() ) {
                    // Send data
                    int len = epSock->send();
                    if (Q_UNLIKELY(len < 0)) {
                        TEpoll::instance()->deletePoll(epSock);
                        epSock->close();
                        epSock->deleteLater();
                        continue;
                    }
                }

                if ( TEpoll::instance()->canReceive() ) {
                    if (TActionWorker::workerCount() >= maxWorkers) {
                        // not receive
                        TEpoll::instance()->modifyPoll(epSock, (EPOLLIN | EPOLLOUT | EPOLLET));  // reset
                        continue;
                    }

                    // Receive data
                    int len = epSock->recv();
                    if (Q_UNLIKELY(len < 0)) {
                        TEpoll::instance()->deletePoll(epSock);
                        epSock->close();
                        epSock->deleteLater();
                        continue;
                    }

                    if (epSock->canReadRequest()) {
#if 1  //TODO: delete here for HTTP 2.0 support
                        // Stop receiving, otherwise the responses is sometimes
                        // placed in the wrong order in case of HTTP-pipeline.
                        TEpoll::instance()->modifyPoll(epSock, (EPOLLOUT | EPOLLET));  // reset
#endif
                        emit incomingRequest(epSock);
                    }
                }
            }
        }

        // Check stop flag
        if (stopped) {
            break;
        }
    }

    TEpollSocket::releaseAllSockets();
    TActionWorker::waitForAllDone(10000);
}


void TMultiplexingServer::stop()
{
    if (!stopped) {
        stopped = true;

        if (isRunning()) {
            QThread::wait(10000);
        }
        TStaticReleaseThread::exec();
    }
}


/*
 * TWorkerStarter class
 */

TWorkerStarter::~TWorkerStarter()
{ }


void TWorkerStarter::startWorker(TEpollSocket *socket)
{
    //
    // Create worker threads in main thread for signal/slot mechanism!
    //
    socket->startWorker();
}
