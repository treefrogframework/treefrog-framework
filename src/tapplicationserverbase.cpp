/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tapplicationserverbase.h"
#include <QDateTime>
#include <QDir>
#include <QLibrary>
#include <QList>
#include <TActionContext>
#include <TActionController>
#include <TDispatcher>
#include <TWebApplication>
#ifdef Q_OS_WIN
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
#ifdef Q_OS_UNIX
#include <tfcore_unix.h>
#endif

/*!
  \class TApplicationServerBase
  \brief The TApplicationServerBase class provides functionality common to
  an web application server.
*/

namespace {
QList<QLibrary *> libsLoaded;
QDateTime loadedTimestamp;
}


bool TApplicationServerBase::loadLibraries()
{
    bool ret = true;

    // Loads libraries
    if (libsLoaded.isEmpty()) {
        // Sets work directory
        QString libPath = Tf::app()->libPath();
        if (!QDir(libPath).exists()) {
            tSystemError("lib directory not found");
            return false;
        }

        // To resolve the symbols in the app libraries
        QDir::setCurrent(libPath);

#if defined(Q_OS_WIN)
        const QStringList libs = {"controller", "view"};
#else
        const QStringList libs = {"libcontroller", "libview"};
#endif

        for (auto &libname : libs) {
            auto lib = new QLibrary(libPath + libname);
            if (lib->load()) {
                tSystemDebug("Library loaded: %s", qUtf8Printable(lib->fileName()));
                libsLoaded << lib;
            } else {
                tSystemWarn("%s", qUtf8Printable(lib->errorString()));
                ret = false;
                unloadLibraries();
                break;
            }
        }

        QStringList controllers = TActionController::availableControllers();
        tSystemDebug("Available controllers: %s", qUtf8Printable(controllers.join(" ")));

        if (ret) {
            loadedTimestamp = latestLibraryTimestamp();
        }
    }
    QDir::setCurrent(Tf::app()->webRootPath());

    return ret;
}


void TApplicationServerBase::unloadLibraries()
{
    for (auto lib : libsLoaded) {
        lib->unload();
        tSystemDebug("Library unloaded: %s", qUtf8Printable(lib->fileName()));
    }
    libsLoaded.clear();
}


QDateTime TApplicationServerBase::latestLibraryTimestamp()
{
#if defined(Q_OS_WIN)
    const QStringList libs = {"controller", "model", "view", "helper"};
#elif defined(Q_OS_LINUX)
    const QStringList libs = {"libcontroller.so", "libmodel.so", "libview.so", "libhelper.so"};
#elif defined(Q_OS_DARWIN)
    const QStringList libs = {"libcontroller.dylib", "libmodel.dylib", "libview.dylib", "libhelper.dylib"};
#else
    const QStringList libs = {"libcontroller.so", "libmodel.so", "libview.so", "libhelper.so"};
#endif

    QDateTime ret = QDateTime::fromSecsSinceEpoch(0);

    QString libPath = Tf::app()->libPath();
    for (auto &lib : libs) {
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


QPair<QHostAddress, quint16> TApplicationServerBase::getPeerInfo(int socketDescriptor)
{
    auto peerInfo = QPair<QHostAddress, quint16>(QHostAddress(), 0);

    union {
        sockaddr a;
        sockaddr_in a4;
        sockaddr_in6 a6;
    } sa;
    socklen_t sasize = sizeof(sa);

    memset(&sa, 0, sizeof(sa));
    if (socketDescriptor <= 0 || ::getpeername(socketDescriptor, &sa.a, &sasize) < 0) {
        return peerInfo;
    }

    if (sa.a.sa_family == AF_INET6) {
        // IPv6
        Q_IPV6ADDR tmp;
        memcpy(&tmp, &sa.a6.sin6_addr, sizeof(tmp));
        peerInfo.first.setAddress(tmp);
        peerInfo.second = ntohs(sa.a6.sin6_port);
    } else {
        // IPv4
        peerInfo.first.setAddress(ntohl(sa.a4.sin_addr.s_addr));
        peerInfo.second = ntohs(sa.a4.sin_port);
    }
    return peerInfo;
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
{
}


TApplicationServerBase::~TApplicationServerBase()
{
    nativeSocketCleanup();
}
