/* Copyright (c) 2015-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tbasictimer.h"
#include "tsystemglobal.h"


TBasicTimer::TBasicTimer(QObject *parent)
    : QObject(parent), QBasicTimer(), interval_(0), receiver_(nullptr)
{ }


void TBasicTimer::start()
{
    tSystemDebug("TBasicTimer::start");
    if (receiver_ && interval_ > 0) {
        QBasicTimer::start(interval_, receiver_);
    }
}


void TBasicTimer::start(int msec)
{
    if (receiver_ && msec > 0) {
        interval_ = msec;
        QBasicTimer::start(interval_, receiver_);
    }
}
