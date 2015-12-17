/* Copyright (c) 2010-2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QLibrary>
#include <QList>
#include <QDir>
#include <QDateTime>
#include <TWebApplication>
#include <TActionContext>
#include <TDispatcher>
#include <TActionController>
#include "tapplicationserverbase.h"
#include "tsqldatabasepool.h"
#include "tkvsdatabasepool.h"
#include "turlroute.h"
#include "tsystemglobal.h"
#include "tsystembus.h"
#include "tpublisher.h"

/*!
  \class TApplicationServerBase
  \brief The TApplicationServerBase class provides functionality common to
  an web application server.
*/

static QList<QLibrary*> libsLoaded;
static QDateTime loadedTimestamp;


bool TApplicationServerBase::loadLibraries()
{
    T_TRACEFUNC("");

    // Loads libraries
    if (libsLoaded.isEmpty()) {
        // Sets work directory
        QString libPath = Tf::app()->libPath();
        if (QDir(libPath).exists()) {
            // To resolve the symbols in the app libraries
            QDir::setCurrent(libPath);
        } else {
            tSystemError("lib directory not found");
            return false;
        }

        loadedTimestamp = latestLibraryTimestamp();

#if defined(Q_OS_WIN)
        QStringList libs = { "controller", "view" };
#elif defined(Q_OS_LINUX)
        QStringList libs = { "libcontroller.so", "libview.so" };
#elif defined(Q_OS_DARWIN)
        QStringList libs = { "libcontroller.dylib", "libview.dylib" };
#else
        QStringList libs = { "libcontroller.so", "libview.so" };
#endif

        for (const auto &libname : libs) {
            auto lib = new QLibrary(libname);
            if (lib->load()) {
                tSystemDebug("Library loaded: %s", qPrintable(lib->fileName()));
                libsLoaded << lib;
            } else {
                tSystemWarn("%s", qPrintable(lib->errorString()));
            }
        }

        QStringList controllers = TActionController::availableControllers();
        tSystemDebug("Available controllers: %s", qPrintable(controllers.join(" ")));
    }
    QDir::setCurrent(Tf::app()->webRootPath());

    TSystemBus::instantiate();
    TPublisher::instantiate();
    TUrlRoute::instantiate();
    TSqlDatabasePool::instantiate();
    TKvsDatabasePool::instantiate();
    return true;
}


QDateTime TApplicationServerBase::latestLibraryTimestamp()
{
#if defined(Q_OS_WIN)
    QStringList libs = { "controller", "model", "view", "helper" };
#elif defined(Q_OS_LINUX)
    QStringList libs = { "libcontroller.so", "libmodel.so", "libview.so", "libhelper.so" };
#elif defined(Q_OS_DARWIN)
    QStringList libs = { "libcontroller.dylib", "libmodel.dylib", "libview.dylib", "libhelper.dylib" };
#else
    QStringList libs = { "libcontroller.so", "libmodel.so", "libview.so", "libhelper.so" };
#endif

    QDateTime ret = QDateTime::fromTime_t(0);

    QString libPath = Tf::app()->libPath();
    for (auto lib : libs) {
        QFileInfo fi(libPath + lib);
        if (fi.isFile() && fi.lastModified() > ret) {
            ret = fi.lastModified();
        }
    }
    return ret;
}


bool TApplicationServerBase::newerLibraryExists()
{
    return (latestLibraryTimestamp() > loadedTimestamp);
}


void TApplicationServerBase::invokeStaticInitialize()
{
    // Calls staticInitialize()
    TDispatcher<TActionController> dispatcher("applicationcontroller");
    bool dispatched = dispatcher.invoke("staticInitialize", QStringList(), Qt::DirectConnection);
    if (!dispatched) {
        tSystemWarn("No such method: staticInitialize() of ApplicationController");
    }
}


void TApplicationServerBase::invokeStaticRelease()
{
    // Calls staticRelease()
    TDispatcher<TActionController> dispatcher("applicationcontroller");
    bool dispatched = dispatcher.invoke("staticRelease", QStringList(), Qt::DirectConnection);
    if (!dispatched) {
        tSystemDebug("No such method: staticRelease() of ApplicationController");
    }
}


TApplicationServerBase::TApplicationServerBase()
{ }


TApplicationServerBase::~TApplicationServerBase()
{
    nativeSocketCleanup();
}
