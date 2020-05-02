/* Copyright (c) 2011-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "processinfo.h"
#include <QtCore>
#include <TGlobal>

namespace TreeFrog {


ProcessInfo::ProcessInfo(qint64 pid) :
    processId(pid)
{
}


bool ProcessInfo::waitForTerminated(int msecs)
{
    if (processId <= 0)
        return false;

    bool ext;
    QTime t;
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


QList<qint64> ProcessInfo::childProcessIds() const
{
    QList<qint64> ids;
    QList<qint64> allPids = allConcurrentPids();

    for (qint64 p : allPids) {
        if (ProcessInfo(p).ppid() == pid()) {
            ids << p;
        }
    }
    return ids;
}


void ProcessInfo::kill(qint64 ppid)
{
    ProcessInfo(ppid).kill();
}


void ProcessInfo::kill(QList<qint64> pids)
{
    for (qint64 pid : pids) {
        ProcessInfo(pid).kill();
    }
}


QList<qint64> ProcessInfo::pidsOf(const QString &processName)
{
    QList<qint64> ret;
    QList<qint64> pids = allConcurrentPids();

    for (auto pid : pids) {
        ProcessInfo pi(pid);
        if (pi.processName() == processName) {
            ret << pi.pid();
        }
    }
    return ret;
}


}  // namespace TreeFrog
