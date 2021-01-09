/* Copyright (c) 2011-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tprocessinfo.h"
#include <QtCore>
#include <TGlobal>


TProcessInfo::TProcessInfo(qint64 pid) :
    processId(pid)
{
}


bool TProcessInfo::waitForTerminated(int msecs)
{
    if (processId <= 0)
        return false;

    bool ext;
    QElapsedTimer t;
    t.start();

    for (;;) {
        ext = exists();
        if (!ext) {
            processId = -1;
            break;
        }

        if (t.elapsed() > msecs)
            break;

        Tf::msleep(100);
    }
    return !ext;
}


QList<qint64> TProcessInfo::childProcessIds() const
{
    QList<qint64> ids;
    const QList<qint64> allPids = allConcurrentPids();

    for (qint64 p : allPids) {
        if (TProcessInfo(p).ppid() == pid()) {
            ids << p;
        }
    }
    return ids;
}


void TProcessInfo::kill(qint64 ppid)
{
    TProcessInfo(ppid).kill();
}


void TProcessInfo::kill(QList<qint64> pids)
{
    for (qint64 pid : (const QList<qint64> &)pids) {
        TProcessInfo(pid).kill();
    }
}


QList<qint64> TProcessInfo::pidsOf(const QString &processName)
{
    QList<qint64> ret;
    const QList<qint64> pids = allConcurrentPids();

    for (auto pid : pids) {
        TProcessInfo pi(pid);
        if (pi.processName() == processName) {
            ret << pi.pid();
        }
    }
    return ret;
}
