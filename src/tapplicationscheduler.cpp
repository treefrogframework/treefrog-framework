/* Copyright (c) 2013-2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TApplicationScheduler>
#include <TWebApplication>
#include "tsystemglobal.h"

/*!
  \class TApplicationScheduler
  \brief The TApplicationScheduler class provides functionality for job
  scheduler. Jobs scheduled by this class will be executed in only one
  application server process.
*/

/*!
  Constructor.
*/
TApplicationScheduler::TApplicationScheduler(QObject *parent)
    : TScheduler(parent)
{ }


TApplicationScheduler::~TApplicationScheduler()
{ }


void TApplicationScheduler::start(int msec)
{
    if (Tf::app()->applicationServerId() == 0) {
        // Starts where applicaraion server ID is 0
        TScheduler::start(msec);
        tSystemDebug("TApplicationScheduler::start msec:%d", msec);
    }
}


void TApplicationScheduler::stop()
{
    if (Tf::app()->applicationServerId() == 0) {
        TScheduler::stop();
    }
}


int TApplicationScheduler::interval() const
{
    return TScheduler::interval();
}


bool TApplicationScheduler::isSingleShot() const
{
    return TScheduler::isSingleShot();
}


void TApplicationScheduler::setSingleShot(bool singleShot)
{
    TScheduler::setSingleShot(singleShot);
}


void TApplicationScheduler::rollbackTransaction()
{
    TScheduler::rollbackTransaction();
}
