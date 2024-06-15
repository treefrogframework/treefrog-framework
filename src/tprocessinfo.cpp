/* Copyright (c) 2011-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tprocessinfo.h"
#include <QtCore>
#include <TGlobal>


TProcessInfo::TProcessInfo(int64_t pid) :
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


QList<int64_t> TProcessInfo::childProcessIds() const
{
    QList<int64_t> ids;
    const QList<int64_t> allPids = allConcurrentPids();

    for (int64_t p : allPids) {
        if (TProcessInfo(p).ppid() == pid()) {
            ids << p;
        }
    }
    return ids;
}


void TProcessInfo::kill(int64_t ppid)
{
    TProcessInfo(ppid).kill();
}


void TProcessInfo::kill(QList<int64_t> pids)
{
    for (int64_t pid : (const QList<int64_t> &)pids) {
        TProcessInfo(pid).kill();
    }
}


QList<int64_t> TProcessInfo::pidsOf(const QString &processName)
{
    QList<int64_t> ret;
    const QList<int64_t> pids = allConcurrentPids();

    for (auto pid : pids) {
        TProcessInfo pi(pid);
        if (pi.processName() == processName) {
            ret << pi.pid();
        }
    }
    return ret;
}
