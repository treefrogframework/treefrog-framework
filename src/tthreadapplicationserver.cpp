/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsystemglobal.h"
#include <TActionThread>
#include <TThreadApplicationServer>
#include <TWebApplication>


/*!
  \class TThreadApplicationServer
  \brief The TThreadApplicationServer class provides functionality common to
  an web application server for thread.
*/

TStack<TActionThread *> *TThreadApplicationServer::threadPoolPtr()
{
    static TStack<TActionThread *> threadPool;
    return &threadPool;
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
        QObject::timerEvent(event);
    } else {
        if (newerLibraryExists()) {
            tSystemInfo("Detect new library of application. Reloading the libraries.");
            Tf::app()->exit(127);
        }
    }
}
