/* Copyright (c) 2011-2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QtCore>
#include <TGlobal>
#include "processinfo.h"

namespace TreeFrog {


ProcessInfo::ProcessInfo(qint64 pid)
    : processId(pid)
{ }


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


QList<qint64> ProcessInfo::pidsOf(const QString &processName)
{
    QList<qint64> ret;
    QList<qint64> pids = allConcurrentPids();
    for (QListIterator<qint64> it(pids); it.hasNext(); ) {
        ProcessInfo pi(it.next());
        if (pi.processName() == processName) {
            ret << pi.pid();
        }
    }
    return ret;
}


QList<qint64> ProcessInfo::killProcesses(const QString &name)
{
    QList<qint64> pids = pidsOf(name);
    for (QListIterator<qint64> it(pids); it.hasNext(); ) {
        const qint64 &pid = it.next();
        ProcessInfo pi(pid);
        pi.kill();
    }
    return pids;
}

} // namespace TreeFrog
