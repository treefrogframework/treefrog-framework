/* Copyright (c) 2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tscheduler.h"
#include <TWebApplication>
#include "tsystemglobal.h"


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
    if (Tf::app()->multiProcessingModule() == TWebApplication::Prefork) {
        tError("Unsupported TScheduler in prefork MPM");
        return;
    }

    timer->start(msec);
}


int	TScheduler::interval() const
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
        TActionContext::rollbackTransactions();
    } else {
        TActionContext::commitTransactions();
    }

    TActionContext::release();
}
