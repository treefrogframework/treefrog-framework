/* Copyright (c) 2010-2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TThreadApplicationServer>
#include <TWebApplication>
#include <TActionThread>
#include "tsystemglobal.h"

/*!
  \class TThreadApplicationServer
  \brief The TThreadApplicationServer class provides functionality common to
  an web application server for thread.
*/

TThreadApplicationServer::TThreadApplicationServer(QObject *parent)
    : QTcpServer(parent), TApplicationServerBase()
{
    maxServers = Tf::app()->maxNumberOfServers();
    connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(terminate()));

    Q_ASSERT(Tf::app()->multiProcessingModule() == TWebApplication::Thread);
}


TThreadApplicationServer::~TThreadApplicationServer()
{ }


bool TThreadApplicationServer::start()
{
    if (isListening()) {
        return true;
    }

    quint16 port = Tf::app()->appSettings().value("ListenPort").toUInt();
    int sock = nativeListen(QHostAddress::Any, port);
    if (sock <= 0 || !setSocketDescriptor(sock)) {
        tSystemError("Failed to set socket descriptor: %d", sock);
        nativeClose(sock);
        return false;
    }
    tSystemDebug("listen successfully.  port:%d", port);

    loadLibraries();

    TStaticInitializeThread *initializer = new TStaticInitializeThread();
    initializer->start();
    initializer->wait();
    delete initializer;
    return true;
}


void TThreadApplicationServer::stop()
{
    T_TRACEFUNC("");
    QTcpServer::close();
}


void TThreadApplicationServer::terminate()
{
    stop();
    releaseAllContexts();
}


void TThreadApplicationServer::incomingConnection(
#if QT_VERSION >= 0x050000
    qintptr socketDescriptor)
#else
    int socketDescriptor)
#endif
{
    T_TRACEFUNC("socketDescriptor: %d", socketDescriptor);

    for (;;) {
        if (TActionThread::threadCount() < maxServers) {
            TActionThread *thread = new TActionThread(socketDescriptor);
            connect(thread, SIGNAL(finished()), this, SLOT(deleteActionContext()));
            insertPointer(thread);
            thread->start();
            break;
        }
        Tf::msleep(1);
        qApp->processEvents(QEventLoop::ExcludeSocketNotifiers);
    }
}


void TThreadApplicationServer::deleteActionContext()
{
    deletePointer(reinterpret_cast<TActionThread *>(sender()));
    sender()->deleteLater();
}

