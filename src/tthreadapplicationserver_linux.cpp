/* Copyright (c) 2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tfcore_unix.h"
#include "tkvsdatabasepool.h"
#include "tsqldatabasepool.h"
#include "tsystemglobal.h"
#include <TActionThread>
#include <TAppSettings>
#include <TThreadApplicationServer>
#include <TWebApplication>
#include <thread>


TThreadApplicationServer::TThreadApplicationServer(int listeningSocket, QObject *parent) :
    QThread(parent),
    TApplicationServerBase(),
    listenSocket(listeningSocket),
    reloadTimer()
{
    QString mpm = Tf::appSettings()->value(Tf::MultiProcessingModule).toString().toLower();
    maxThreads = Tf::appSettings()->readValue(QLatin1String("MPM.") + mpm + ".MaxThreadsPerAppServer").toInt();
    if (maxThreads == 0) {
        maxThreads = Tf::appSettings()->readValue(QLatin1String("MPM.") + mpm + ".MaxServers", "128").toInt();
    }
    tSystemDebug("MaxThreads: %d", maxThreads);

    // Thread pooling
    for (int i = 0; i < maxThreads; i++) {
        TActionThread *thread = new TActionThread(0);
        connect(thread, &TActionThread::finished, [=]() {
            threadPoolPtr()->push(thread);
        });
        threadPoolPtr()->push(thread);
    }
}


bool TThreadApplicationServer::start(bool debugMode)
{
    if (QThread::isRunning()) {
        return true;
    }

    bool res = loadLibraries();
    if (!res) {
        if (debugMode) {
            tSystemError("Failed to load application libraries.");
            return false;
        } else {
            tSystemWarn("Failed to load application libraries.");
        }
    }

    if (listenSocket <= 0) {
        tSystemError("Failed to set socket descriptor: %d", listenSocket);
        return false;
    }

    // To work a timer in main thread
    TSqlDatabasePool::instance();
    TKvsDatabasePool::instance();

    TStaticInitializeThread::exec();
    QThread::start();
    return true;
}


void TThreadApplicationServer::stop()
{
    if (!QThread::isRunning()) {
        return;
    }

    stopFlag = true;
    QThread::wait();
    listenSocket = 0;

    if (!isAutoReloadingEnabled()) {
        TActionThread::waitForAllDone(10000);
    }
    TStaticReleaseThread::exec();
}


void TThreadApplicationServer::run()
{
    constexpr int timeout = 500;  // msec
    struct pollfd pfd;

    while (listenSocket > 0 && !stopFlag) {
        pfd.fd = listenSocket;
        pfd.events = POLLIN;
        pfd.revents = 0;
        int ret = tf_poll(&pfd, 1, timeout);

        if (ret < 0) {
            tSystemError("poll error");
            break;
        }

        if (ret > 0 && (pfd.revents & POLLIN)) {
            int socketDescriptor = tf_accept4(listenSocket, nullptr, nullptr, (SOCK_CLOEXEC | SOCK_NONBLOCK));
            if (socketDescriptor > 0) {
                tSystemDebug("incomingConnection  sd:%d  thread count:%d  max:%d", socketDescriptor, TActionThread::threadCount(), maxThreads);
                TActionThread *thread;

                while (!threadPoolPtr()->pop(thread)) {
                    std::this_thread::yield();
                    Tf::msleep(1);
                }

                tSystemDebug("thread ptr: %lld", (quint64)thread);
                thread->setSocketDescriptor(socketDescriptor);
                thread->start();
            }
        }
    }
}
