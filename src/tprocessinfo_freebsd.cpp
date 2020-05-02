/* Copyright (c) 2017-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tprocessinfo.h"
#include <QtCore>
#include <libprocstat.h>
#include <libutil.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/queue.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <sys/user.h>


bool TProcessInfo::exists() const
{
    return allConcurrentPids().contains(processId);
}


qint64 TProcessInfo::ppid() const
{
    qint64 parentpid = 0;

    if (processId <= 0) {
        return parentpid;
    }

    uint cnt;
    auto *prstat = procstat_open_sysctl();
    auto *procs = procstat_getprocs(prstat, KERN_PROC_PROC, 0, &cnt);

    if (procs) {
        for (uint i = 0; i < cnt; i++) {
            if (procs[i].ki_pid == processId) {
                parentpid = procs[i].ki_ppid;
                break;
            }
        }
        procstat_freeprocs(prstat, procs);
    }
    procstat_close(prstat);
    return parentpid;
}


QString TProcessInfo::processName() const
{
    QString ret;
    struct kinfo_proc *proc = kinfo_getproc(processId);

    if (proc) {
        ret.append(proc->ki_comm);
        free(proc);
    }
    return ret;
}


QList<qint64> TProcessInfo::allConcurrentPids()
{
    QList<qint64> pidList;
    uint cnt;
    auto *prstat = procstat_open_sysctl();
    auto *procs = procstat_getprocs(prstat, KERN_PROC_PROC, 0, &cnt);

    if (procs) {
        for (uint i = 0; i < cnt; i++) {
            qint64 pid = procs[i].ki_pid;
            if (pid > 0) {
                pidList << pid;
            }
        }
        procstat_freeprocs(prstat, procs);
    }
    procstat_close(prstat);

    std::sort(pidList.begin(), pidList.end());  // Sorts the items
    return pidList;
}


void TProcessInfo::terminate()
{
    if (processId > 0) {
        ::kill(processId, SIGTERM);
    }
}


void TProcessInfo::kill()
{
    if (processId > 0) {
        ::kill(processId, SIGKILL);
    }
    processId = -1;
}


void TProcessInfo::restart()
{
    if (processId > 0) {
        ::kill(processId, SIGHUP);
    }
}
