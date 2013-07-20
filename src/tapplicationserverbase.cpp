/* Copyright (c) 2010-2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QLibrary>
#include <QDir>
#include <TWebApplication>
#include <TActionContext>
#include <TSqlDatabasePool>
#include <TKvsDatabasePool>
#include <TDispatcher>
#include <TActionController>
#include "tapplicationserverbase.h"
#include "turlroute.h"
#include "tsystemglobal.h"

/*!
  \class TApplicationServerBase
  \brief The TApplicationServerBase class provides functionality common to
  an web application server.
*/

static bool libLoaded = false;


bool TApplicationServerBase::loadLibraries()
{
    T_TRACEFUNC("");

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
    return true;
}


void TApplicationServerBase::invokeStaticInitialize()
{
    // Calls staticInitialize()
    TDispatcher<TActionController> dispatcher("applicationcontroller");
    bool dispatched = dispatcher.invoke("staticInitialize");
    if (!dispatched) {
        tSystemWarn("No such method: staticInitialize() of ApplicationController");
    }
}


TApplicationServerBase::TApplicationServerBase()
{
    nativeSocketInit();
}


TApplicationServerBase::~TApplicationServerBase()
{
    nativeSocketCleanup();
}


void TApplicationServerBase::insertPointer(TActionContext *p)
{
    QMutexLocker locker(&setMutex);
    actionContexts.insert(p);
}


void TApplicationServerBase::deletePointer(TActionContext *p)
{
    QMutexLocker locker(&setMutex);
    actionContexts.remove(p);
}


void TApplicationServerBase::releaseAllContexts()
{
    if (actionContextCount() > 0) {
        setMutex.lock();
        for (QSetIterator<TActionContext *> i(actionContexts); i.hasNext(); ) {
            i.next()->stop();  // Stops application server
        }
        setMutex.unlock();

        for (;;) {
            Tf::msleep(1);
            qApp->processEvents();

            QMutexLocker locker(&setMutex);
            if (actionContexts.isEmpty()) {
                break;
            }
        }
    }
}


int TApplicationServerBase::actionContextCount() const
{
    QMutexLocker locker(&setMutex);
    return actionContexts.count();
}
