/* Copyright (c) 2010-2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TPreforkApplicationServer>
#include <TWebApplication>
#include <TActionForkProcess>
#include "tapplicationserverbase.h"
#include "tsystemglobal.h"


class TStaticInitializer : public TActionForkProcess
{
public:
    TStaticInitializer() : TActionForkProcess(0) { }

    void start()
    {
        currentActionContext = this;
        TApplicationServerBase::invokeStaticInitialize();
        currentActionContext = 0;
    }
};


class TStaticReleaser : public TActionForkProcess
{
public:
    TStaticReleaser() : TActionForkProcess(0) { }

    void start()
    {
        currentActionContext = this;
        TApplicationServerBase::invokeStaticRelease();
        currentActionContext = 0;
    }
};

/*!
  \class TPreforkApplicationServer
  \brief The TPreforkApplicationServer class provides functionality common to
  an web application server for prefork.
*/

TPreforkApplicationServer::TPreforkApplicationServer(QObject *parent)
    : QTcpServer(parent), TApplicationServerBase()
{
    Q_ASSERT(Tf::app()->multiProcessingModule() == TWebApplication::Prefork);
}


TPreforkApplicationServer::~TPreforkApplicationServer()
{ }


bool TPreforkApplicationServer::start()
{
    loadLibraries();

    TStaticInitializer *initializer = new TStaticInitializer();
    initializer->start();
    delete initializer;
    return true;
}


void TPreforkApplicationServer::stop()
{
    QTcpServer::close();

    TStaticReleaser *releaser = new TStaticReleaser();
    releaser->start();
    delete releaser;
}


void TPreforkApplicationServer::incomingConnection(
#if QT_VERSION >= 0x050000
    qintptr socketDescriptor)
#else
    int socketDescriptor)
#endif
{
    close();  // Closes the listening port
    TActionForkProcess *process = new TActionForkProcess(socketDescriptor);
    connect(process, SIGNAL(finished()), process, SLOT(deleteLater()));
    process->start();
}
