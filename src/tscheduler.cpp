/* Copyright (c) 2013-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tscheduler.h"
#include <TWebApplication>
#include "tpublisher.h"
#include "tsystemglobal.h"

/*!
  \class TScheduler
  \brief The TScheduler class provides functionality for job scheduler.
  Jobs scheduled by this class will be executed in each application server
  process.
*/

TScheduler::TScheduler(QObject *parent)
    : TDatabaseContextThread(parent), timer(new QTimer())
{
    moveToThread(Tf::app()->thread());
    timer->moveToThread(Tf::app()->thread());
    timer->setSingleShot(false);

    QObject::connect(timer, SIGNAL(timeout()), this, SLOT(start()));
    QObject::connect(this, SIGNAL(startTimer(int)), timer, SLOT(start(int)));
    QObject::connect(this, SIGNAL(startTimer()), timer, SLOT(start()));
    QObject::connect(this, SIGNAL(stopTimer()), timer, SLOT(stop()));
}


TScheduler::~TScheduler()
{
    delete timer;
}


void TScheduler::start(int msec)
{
    emit startTimer(msec);
}


void TScheduler::restart()
{
    emit startTimer();
}


void TScheduler::stop()
{
    emit stopTimer();
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


void TScheduler::publish(const QString &topic, const QString &text)
{
    TPublisher::instance()->publish(topic, text, nullptr);
}


void TScheduler::publish(const QString &topic, const QByteArray &binary)
{
    TPublisher::instance()->publish(topic, binary, nullptr);
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
