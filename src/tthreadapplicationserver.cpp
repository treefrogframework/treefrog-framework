/* Copyright (c) 2010-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TThreadApplicationServer>
#include <TWebApplication>
#include <TAppSettings>
#include <TActionThread>
#include "tsqldatabasepool.h"
#include "tkvsdatabasepool.h"
#include "turlroute.h"
#include "tsystemglobal.h"
#include "tsystembus.h"
#include "tpublisher.h"
#include "tsystemglobal.h"

/*!
  \class TThreadApplicationServer
  \brief The TThreadApplicationServer class provides functionality common to
  an web application server for thread.
*/

TThreadApplicationServer::TThreadApplicationServer(int listeningSocket, QObject *parent) :
    QTcpServer(parent),
    TApplicationServerBase(),
    listenSocket(listeningSocket),
    maxThreads(0),
    reloadTimer()
{
    QString mpm = Tf::appSettings()->value(Tf::MultiProcessingModule).toString().toLower();
    maxThreads = Tf::appSettings()->readValue(QLatin1String("MPM.") + mpm + ".MaxThreadsPerAppServer").toInt();
    if (maxThreads == 0) {
        maxThreads = Tf::appSettings()->readValue(QLatin1String("MPM.") + mpm + ".MaxServers", "128").toInt();
    }
    tSystemDebug("MaxThreads: %d", maxThreads);

    Q_ASSERT(Tf::app()->multiProcessingModule() == TWebApplication::Thread);
}


TThreadApplicationServer::~TThreadApplicationServer()
{ }


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

    // instantiate
    if (!debugMode) {
        TSystemBus::instantiate();
        TPublisher::instantiate();
    }
    TUrlRoute::instantiate();
    TSqlDatabasePool::instantiate();
    TKvsDatabasePool::instantiate();

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
    for (;;) {
        tSystemDebug("incomingConnection  sd:%lld  thread count:%d  max:%d", (qint64)socketDescriptor, TActionThread::threadCount(), maxThreads);
        if (TActionThread::threadCount() < maxThreads) {
            TActionThread *thread = new TActionThread(socketDescriptor);
            connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
            thread->start();
            break;
        }
        Tf::msleep(1);
        qApp->processEvents(QEventLoop::ExcludeSocketNotifiers);
    }
}


void TThreadApplicationServer::setAutoReloadingEnabled(bool enable)
{
    if (enable) {
        reloadTimer.start(500, this);
    } else {
        reloadTimer.stop();
    }
}


bool TThreadApplicationServer::isAutoReloadingEnabled()
{
    return reloadTimer.isActive();
}


void TThreadApplicationServer::timerEvent(QTimerEvent *event)
{
    if (event->timerId() != reloadTimer.timerId()) {
        QTcpServer::timerEvent(event);
    } else {
        if (newerLibraryExists()) {
            tSystemInfo("Detect new library of application. Reloading the libraries.");
            Tf::app()->exit(127);
        }
    }
}
