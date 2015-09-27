/* Copyright (c) 2013-2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tscheduler.h"
#include <TWebApplication>
#include "tsystemglobal.h"

/*!
  \class TScheduler
  \brief The TScheduler class provides functionality for job scheduler.
  Jobs scheduled by this class will be executed in each application server
  process.
*/

TScheduler::TScheduler(QObject *parent)
    : QThread(parent), timer(new QTimer()), rollback(false)
{
    moveToThread(Tf::app()->thread());
    timer->moveToThread(Tf::app()->thread());
    timer->setSingleShot(false);

    QObject::connect(timer, SIGNAL(timeout()), this, SLOT(start()));
}


TScheduler::~TScheduler()
{
    delete timer;
}


void TScheduler::start(int msec)
{
    timer->start(msec);
}


void TScheduler::stop()
{
    timer->stop();

    if (QThread::isRunning()) {
        QThread::wait();
    }
}


int TScheduler::interval() const
{
    return timer->interval();
}


bool TScheduler::isSingleShot() const
{
    return timer->isSingleShot();
}


void TScheduler::setSingleShot(bool singleShot)
{
    timer->setSingleShot(singleShot);
}


void TScheduler::rollbackTransaction()
{
    rollback = true;
}


void TScheduler::start(Priority priority)
{
    QThread::start(priority);
}


void TScheduler::run()
{
    rollback = false;

    // Executes the job
    job();

    if (rollback) {
        TDatabaseContext::rollbackTransactions();
    } else {
        TDatabaseContext::commitTransactions();
    }

    TDatabaseContext::release();
}
