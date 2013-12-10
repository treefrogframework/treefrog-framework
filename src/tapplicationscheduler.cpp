/* Copyright (c) 2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TApplicationScheduler>
#include <TWebApplication>
#include "tsystemglobal.h"


TApplicationScheduler::TApplicationScheduler()
    : TScheduler()
{ }


TApplicationScheduler::~TApplicationScheduler()
{ }


void TApplicationScheduler::start(int msec)
{
    switch ( Tf::app()->multiProcessingModule() ) {
    case TWebApplication::Prefork:
        tError("Unsupported TApplicationScheduler in prefork MPM");
        break;

    case TWebApplication::Thread:
        TScheduler::start(msec);
        tSystemDebug("TApplicationScheduler::start msec:%d", msec);
        break;

    case TWebApplication::Hybrid:
        if (Tf::app()->applicationServerId() == 0) {
            // Starts where applicaraion server ID is 0
            TScheduler::start(msec);
            tSystemDebug("TApplicationScheduler::start msec:%d", msec);
        }
        break;

    default:
        break;
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
