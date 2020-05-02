/* Copyright (c) 2015-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tbasictimer.h"
#include "tsystemglobal.h"


TBasicTimer::TBasicTimer(QObject *parent) :
    QObject(parent),
    QBasicTimer()
{
}


void TBasicTimer::start()
{
    tSystemDebug("TBasicTimer::start");
    if (_receiver && _interval > 0) {
        QBasicTimer::start(_interval, _receiver);
    }
}


void TBasicTimer::start(int msec)
{
    if (_receiver && msec > 0) {
        _interval = msec;
        QBasicTimer::start(_interval, _receiver);
    }
}
