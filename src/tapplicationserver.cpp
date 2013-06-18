/* Copyright (c) 2010-2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QLibrary>
#include <QDir>
#include <TApplicationServer>
#include <TWebApplication>
#include <TActionThread>
#include <TActionForkProcess>
#include <TSqlDatabasePool>
#include <TKvsDatabasePool>
#include <TDispatcher>
#include <TActionController>
#include "turlroute.h"
#include "tsystemglobal.h"


static void invokeStaticInitialize()
{
    // Calls staticInitialize()
    TDispatcher<TActionController> dispatcher("applicationcontroller");
    bool dispatched = dispatcher.invoke("staticInitialize");
    if (!dispatched) {
        tSystemWarn("No such method: staticInitialize() of ApplicationController");
    }
}


class TStaticInitializer : public TActionForkProcess
{
public:
    TStaticInitializer() : TActionForkProcess(0) { }

    void start()
    {
        currentActionContext = this;
        invokeStaticInitialize();
        currentActionContext = 0;
    }
};


class TStaticInitializeThread : public TActionThread
{
public:
    TStaticInitializeThread() : TActionThread(0) { }
protected:
    void run()
    {
        invokeStaticInitialize();
    }
};

/*!
  \class TApplicationServer
  \brief The TApplicationServer class provides functionality common to
  an web application server.
*/

static bool libLoaded = false;


TApplicationServer::TApplicationServer(QObject *parent)
    : QTcpServer(parent)
{
    nativeSocketInit();

    maxServers = Tf::app()->maxNumberOfServers();
    connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(terminate()));
}


TApplicationServer::~TApplicationServer()
{
    nativeSocketCleanup();
}


bool TApplicationServer::open()
{
    T_TRACEFUNC("");

    if (!isListening()) {
        quint16 port = Tf::app()->appSettings().value("ListenPort").toUInt();
        int sock = nativeListen(QHostAddress::Any, port);
        if (sock > 0 && setSocketDescriptor(sock)) {
            tSystemDebug("listen successfully.  port:%d", port);
        } else {
            tSystemError("Failed to set socket descriptor: %d", sock);
            nativeClose(sock);
            return false;
        }
    }

    // Loads libraries
    if (!libLoaded) {

        // Sets work directory
        QString libPath = Tf::app()->libPath();
        if (QDir(libPath).exists()) {
            // To resolve the symbols in the app libraries
            QDir::setCurrent(libPath);
        } else {
            tSystemError("lib directory not found");
            return false;
        }

        QStringList libs;
#if defined(Q_OS_WIN)
        libs << "controller" << "view";
#else
        libs << "libcontroller" << "libview";
#endif

        for (QStringListIterator it(libs); it.hasNext(); ) {
            QLibrary lib(it.next());
            if (lib.load()) {
                tSystemDebug("Library loaded: %s", qPrintable(lib.fileName()));
                libLoaded = true;
            } else {
                tSystemError("%s", qPrintable(lib.errorString()));
            }
        }

        QStringList controllers = TActionController::availableControllers();
        tSystemDebug("Available controllers: %s", qPrintable(controllers.join(" ")));
    }
    QDir::setCurrent(Tf::app()->webRootPath());

    TUrlRoute::instantiate();
    TSqlDatabasePool::instantiate();
    TKvsDatabasePool::instantiate();

    switch (Tf::app()->multiProcessingModule()) {
    case TWebApplication::Thread: {
        TStaticInitializeThread *initializer = new TStaticInitializeThread();
        initializer->start();
        initializer->wait();
        delete initializer;
        break; }

    case TWebApplication::Prefork: {
        TStaticInitializer *initializer = new TStaticInitializer();
        initializer->start();
        delete initializer;
        break; }

    default:
        break;
    }

    return true;
}


bool TApplicationServer::isOpen() const
{
    return isListening();
}


void TApplicationServer::close()
{
    T_TRACEFUNC("");
    QTcpServer::close();
}


void TApplicationServer::terminate()
{
    close();

    if (actionContextCount() > 0) {
        setMutex.lock();
        for (QSetIterator<TActionContext *> i(actionContexts); i.hasNext(); ) {
            i.next()->stop();  // Stops application server
        }
        setMutex.unlock();

        for (;;) {
            qApp->processEvents();
            QMutexLocker locker(&setMutex);
            if (actionContexts.isEmpty()) {
                break;
            }
        }
    }
}


void TApplicationServer::incomingConnection(
#if QT_VERSION >= 0x050000
    qintptr socketDescriptor)
#else
    int socketDescriptor)
#endif
{
    T_TRACEFUNC("socketDescriptor: %d", socketDescriptor);

    switch ( Tf::app()->multiProcessingModule() ) {
    case TWebApplication::Thread:
        for (;;) {
            if (actionContextCount() < maxServers) {
                TActionThread *thread = new TActionThread(socketDescriptor);
                connect(thread, SIGNAL(finished()), this, SLOT(deleteActionContext()));
                insertPointer(thread);
                thread->start();
                break;
            }
            Tf::msleep(1);
            qApp->processEvents(QEventLoop::ExcludeSocketNotifiers);
        }
        break;

    case TWebApplication::Prefork: {
        close();  // Closes the listening port
        TActionForkProcess *process = new TActionForkProcess(socketDescriptor);
        connect(process, SIGNAL(finished()), this, SLOT(deleteActionContext()));
        insertPointer(process);
        process->start();
        break; }

    default:
        break;
    }
}


void TApplicationServer::deleteActionContext()
{
    T_TRACEFUNC("");
    QMutexLocker locker(&setMutex);
    actionContexts.remove(reinterpret_cast<TActionThread *>(sender()));
    sender()->deleteLater();
}


void TApplicationServer::insertPointer(TActionContext *p)
{
    QMutexLocker locker(&setMutex);
    actionContexts.insert(p);
}


int TApplicationServer::actionContextCount() const
{
    QMutexLocker locker(&setMutex);
    return actionContexts.count();
}
