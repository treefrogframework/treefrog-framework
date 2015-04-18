/* Copyright (c) 2010-2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TThreadApplicationServer>
#include <TWebApplication>
#include <TAppSettings>
#include <TActionThread>
#include "tsystemglobal.h"

/*!
  \class TThreadApplicationServer
  \brief The TThreadApplicationServer class provides functionality common to
  an web application server for thread.
*/

TThreadApplicationServer::TThreadApplicationServer(int listeningSocket, QObject *parent)
    : QTcpServer(parent), TApplicationServerBase(), listenSocket(listeningSocket), maxThreads(0)
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


bool TThreadApplicationServer::start()
{
    if (isListening()) {
        return true;
    }

    if (listenSocket <= 0 || !setSocketDescriptor(listenSocket)) {
        tSystemError("Failed to set socket descriptor: %d", listenSocket);
        return false;
    }

    loadLibraries();
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

    TActionThread::waitForAllDone(10000);
    TStaticReleaseThread::exec();
}


void TThreadApplicationServer::incomingConnection(
#if QT_VERSION >= 0x050000
    qintptr socketDescriptor)
#else
    int socketDescriptor)
#endif
{
    for (;;) {
        tSystemDebug("incomingConnection  sd:%d  thread count:%d  max:%d", socketDescriptor, TActionThread::threadCount(), maxThreads);
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
