/* Copyright (c) 2011-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "processinfo.h"
#include <QtCore>
#include <signal.h>
#include <sys/sysctl.h>
#include <sys/types.h>

namespace TreeFrog {


bool ProcessInfo::exists() const
{
    return allConcurrentPids().contains(processId);
}


int64_t ProcessInfo::ppid() const
{
    int64_t ppid = 0;
    struct kinfo_proc kp;
    size_t bufSize = sizeof(struct kinfo_proc);
    int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PID, (int)processId};

    if (sysctl(mib, 4, &kp, &bufSize, NULL, 0) == 0) {
        ppid = kp.kp_eproc.e_ppid;
    }
    return ppid;
}


QString ProcessInfo::processName() const
{
    QString ret;
    struct kinfo_proc kp;
    size_t bufSize = sizeof(struct kinfo_proc);
    int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PID, (int)processId};

    if (sysctl(mib, 4, &kp, &bufSize, NULL, 0) == 0) {
        ret.append(kp.kp_proc.p_comm);
    }
    return ret;
}


QList<int64_t> ProcessInfo::allConcurrentPids()
{
    QList<int64_t> ret;
    struct kinfo_proc *kp;
    size_t bufSize = 0;
    int mib[3] = {CTL_KERN, KERN_PROC, KERN_PROC_ALL};

    if (sysctl(mib, 3, NULL, &bufSize, NULL, 0) == 0) {
        kp = (struct kinfo_proc *)new char[bufSize];
        if (sysctl(mib, 3, kp, &bufSize, NULL, 0) == 0) {
            for (size_t i = 0; i < (bufSize / sizeof(struct kinfo_proc)); ++i) {
                int64_t pid = kp[i].kp_proc.p_pid;
                if (pid > 0)
                    ret.prepend(pid);
            }
        }
        delete[] kp;
    }

    std::sort(ret.begin(), ret.end());  // Sorts the items
    return ret;
}


void ProcessInfo::terminate()
{
    if (processId > 0) {
        ::kill(processId, SIGTERM);
    }
}


void ProcessInfo::kill()
{
    if (processId > 0) {
        ::kill(processId, SIGKILL);
    }
    processId = -1;
}


void ProcessInfo::restart()
{
    if (processId > 0) {
        ::kill(processId, SIGHUP);
    }
}

}  // namespace TreeFrog
