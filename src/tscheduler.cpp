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

TScheduler::TScheduler() :
    TDatabaseContextThread(),
    _timer(new QTimer())
{
    moveToThread(Tf::app()->thread());
    _timer->moveToThread(Tf::app()->thread());
    _timer->setSingleShot(false);

    QObject::connect(_timer, SIGNAL(timeout()), this, SLOT(start()));
    QObject::connect(this, SIGNAL(startTimer(int)), _timer, SLOT(start(int)));
    QObject::connect(this, SIGNAL(startTimer()), _timer, SLOT(start()));
    QObject::connect(this, SIGNAL(stopTimer()), _timer, SLOT(stop()));
}


TScheduler::~TScheduler()
{
    delete _timer;
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
    return _timer->interval();
}


bool TScheduler::isSingleShot() const
{
    return _timer->isSingleShot();
}


void TScheduler::setSingleShot(bool singleShot)
{
    _timer->setSingleShot(singleShot);
}


void TScheduler::rollbackTransaction()
{
    _rollback = true;
}


void TScheduler::publish(const QString &topic, const QString &text)
{
    TPublisher::instance()->publish(topic, text, nullptr);
}


void TScheduler::publish(const QString &topic, const QByteArray &binary)
{
    TPublisher::instance()->publish(topic, binary, nullptr);
}


void TScheduler::run()
{
    _rollback = false;
    TDatabaseContext::setCurrentDatabaseContext(this);

    // Executes the job
    job();

    if (_rollback) {
        TDatabaseContext::rollbackTransactions();
    } else {
        TDatabaseContext::commitTransactions();
    }

    TDatabaseContext::release();
    TDatabaseContext::setCurrentDatabaseContext(nullptr);

    if (_autoDelete && !_timer->isActive()) {
        connect(this, &TScheduler::finished, this, &QObject::deleteLater);
    }
}
