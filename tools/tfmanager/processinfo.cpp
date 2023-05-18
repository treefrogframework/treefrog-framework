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


ProcessInfo::ProcessInfo(int64_t pid) :
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


QList<int64_t> ProcessInfo::childProcessIds() const
{
    QList<int64_t> ids;
    QList<int64_t> allPids = allConcurrentPids();

    for (int64_t p : allPids) {
        if (ProcessInfo(p).ppid() == pid()) {
            ids << p;
        }
    }
    return ids;
}


void ProcessInfo::kill(int64_t ppid)
{
    ProcessInfo(ppid).kill();
}


void ProcessInfo::kill(QList<int64_t> pids)
{
    for (int64_t pid : pids) {
        ProcessInfo(pid).kill();
    }
}


QList<int64_t> ProcessInfo::pidsOf(const QString &processName)
{
    QList<int64_t> ret;
    QList<int64_t> pids = allConcurrentPids();

    for (auto pid : pids) {
        ProcessInfo pi(pid);
        if (pi.processName() == processName) {
            ret << pi.pid();
        }
    }
    return ret;
}


}  // namespace TreeFrog
