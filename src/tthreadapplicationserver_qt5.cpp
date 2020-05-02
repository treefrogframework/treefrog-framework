/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsystemglobal.h"
#include <TActionThread>
#include <TAppSettings>
#include <TThreadApplicationServer>
#include <TWebApplication>
#include <thread>


TThreadApplicationServer::TThreadApplicationServer(int listeningSocket, QObject *parent) :
    QTcpServer(parent),
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

    Q_ASSERT(Tf::app()->multiProcessingModule() == TWebApplication::Thread);
}


bool TThreadApplicationServer::start(bool debugMode)
{
    if (isListening()) {
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

    if (listenSocket <= 0 || !setSocketDescriptor(listenSocket)) {
        tSystemError("Failed to set socket descriptor: %d", listenSocket);
        return false;
    }

    TStaticInitializeThread::exec();
    return true;
}


void TThreadApplicationServer::stop()
{
    if (!isListening()) {
        return;
    }

    QTcpServer::close();
    listenSocket = 0;

    if (!isAutoReloadingEnabled()) {
        TActionThread::waitForAllDone(10000);
    }
    TStaticReleaseThread::exec();
}


void TThreadApplicationServer::incomingConnection(qintptr socketDescriptor)
{
    tSystemDebug("incomingConnection  sd:%lld  thread count:%d  max:%d", (qint64)socketDescriptor, TActionThread::threadCount(), maxThreads);
    TActionThread *thread;
    while (!threadPoolPtr()->pop(thread)) {
        std::this_thread::yield();
        //qApp->processEvents(QEventLoop::ExcludeSocketNotifiers);
        Tf::msleep(1);
    }
    tSystemDebug("thread ptr: %lld", (quint64)thread);
    thread->setSocketDescriptor(socketDescriptor);
    thread->start();
}
