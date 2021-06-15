/* Copyright (c) 2013-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tjobscheduler.h"
#include "tpublisher.h"
#include "tsystemglobal.h"
#include <TWebApplication>

/*!
  \class TJobScheduler
  \brief The TJobScheduler class provides functionality for job scheduler.
  Jobs scheduled by this class will be executed in each application server
  process.
*/

TJobScheduler::TJobScheduler() :
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


TJobScheduler::~TJobScheduler()
{
    delete _timer;
}


void TJobScheduler::start(int msec)
{
    if (Tf::app()->applicationServerId() == 0) {
        // Starts where applicaraion server ID is 0
        emit startTimer(msec);
        tSystemDebug("TJobScheduler::start msec:%d", msec);
    }
}


void TJobScheduler::restart()
{
    if (Tf::app()->applicationServerId() == 0) {
        emit startTimer();
        tSystemDebug("TJobScheduler::restart");
    }
}


void TJobScheduler::stop()
{
    if (Tf::app()->applicationServerId() == 0) {
        emit stopTimer();
        tSystemDebug("TJobScheduler::stop");
    }
}


int TJobScheduler::interval() const
{
    return _timer->interval();
}


bool TJobScheduler::isSingleShot() const
{
    return _timer->isSingleShot();
}


void TJobScheduler::setSingleShot(bool singleShot)
{
    _timer->setSingleShot(singleShot);
}


void TJobScheduler::rollbackTransaction()
{
    _rollback = true;
}


void TJobScheduler::publish(const QString &topic, const QString &text)
{
    TPublisher::instance()->publish(topic, text, nullptr);
}


void TJobScheduler::publish(const QString &topic, const QByteArray &binary)
{
    TPublisher::instance()->publish(topic, binary, nullptr);
}


void TJobScheduler::run()
{
    _rollback = false;
    TDatabaseContext::setCurrentDatabaseContext(this);

    try {
        // Executes the job
        job();

        if (_rollback) {
            TDatabaseContext::rollbackTransactions();
        } else {
            TDatabaseContext::commitTransactions();
        }

    } catch (ClientErrorException &e) {
        tWarn("Caught ClientErrorException: status code:%d", e.statusCode());
        tSystemWarn("Caught ClientErrorException: status code:%d", e.statusCode());
    } catch (SqlException &e) {
        tError("Caught SqlException: %s  [%s:%d]", qUtf8Printable(e.message()), qUtf8Printable(e.fileName()), e.lineNumber());
        tSystemError("Caught SqlException: %s  [%s:%d]", qUtf8Printable(e.message()), qUtf8Printable(e.fileName()), e.lineNumber());
    } catch (KvsException &e) {
        tError("Caught KvsException: %s  [%s:%d]", qUtf8Printable(e.message()), qUtf8Printable(e.fileName()), e.lineNumber());
        tSystemError("Caught KvsException: %s  [%s:%d]", qUtf8Printable(e.message()), qUtf8Printable(e.fileName()), e.lineNumber());
    } catch (SecurityException &e) {
        tError("Caught SecurityException: %s  [%s:%d]", qUtf8Printable(e.message()), qUtf8Printable(e.fileName()), e.lineNumber());
        tSystemError("Caught SecurityException: %s  [%s:%d]", qUtf8Printable(e.message()), qUtf8Printable(e.fileName()), e.lineNumber());
    } catch (RuntimeException &e) {
        tError("Caught RuntimeException: %s  [%s:%d]", qUtf8Printable(e.message()), qUtf8Printable(e.fileName()), e.lineNumber());
        tSystemError("Caught RuntimeException: %s  [%s:%d]", qUtf8Printable(e.message()), qUtf8Printable(e.fileName()), e.lineNumber());
    } catch (StandardException &e) {
        tError("Caught StandardException: %s  [%s:%d]", qUtf8Printable(e.message()), qUtf8Printable(e.fileName()), e.lineNumber());
        tSystemError("Caught StandardException: %s  [%s:%d]", qUtf8Printable(e.message()), qUtf8Printable(e.fileName()), e.lineNumber());
    } catch (std::exception &e) {
        tError("Caught Exception: %s", e.what());
        tSystemError("Caught Exception: %s", e.what());
    }

    TDatabaseContext::release();
    TDatabaseContext::setCurrentDatabaseContext(nullptr);

    if (_autoDelete && !_timer->isActive()) {
        connect(this, &TJobScheduler::finished, this, &QObject::deleteLater);
    }
}
