/* Copyright (c) 2011-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "processinfo.h"
#include <QtCore>
#include <signal.h>
#include <sys/types.h>

namespace TreeFrog {


bool ProcessInfo::exists() const
{
    return QFileInfo(QLatin1String("/proc/") + QString::number(processId) + "/status").exists();
}


qint64 ProcessInfo::ppid() const
{
    const char DIRECTIVE[] = "PPid:";
    QString ppid;

    if (processId > 0) {
        // Read proc
        QFile procfile(QLatin1String("/proc/") + QString::number(processId) + "/status");
        if (procfile.open(QIODevice::ReadOnly)) {
            ppid = QString(procfile.readAll()).split("\n").filter(DIRECTIVE, Qt::CaseInsensitive).value(0).mid(sizeof(DIRECTIVE)).trimmed();
        }
    }
    return ppid.toLongLong();
}


QString ProcessInfo::processName() const
{
    const char DIRECTIVE[] = "Name:";
    QString ret;

    if (processId > 0) {
        // Read proc
        QFile procfile(QLatin1String("/proc/") + QString::number(processId) + "/status");
        if (procfile.open(QIODevice::ReadOnly)) {
            ret = QString(procfile.readAll()).split("\n").filter(DIRECTIVE, Qt::CaseInsensitive).value(0).mid(sizeof(DIRECTIVE)).trimmed();
        }
    }
    return ret;
}


QList<qint64> ProcessInfo::allConcurrentPids()
{
    QList<qint64> ret;
    QDir proc("/proc");
    QStringList dirs = proc.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);

    for (QStringListIterator it(dirs); it.hasNext();) {
        const QString &s = it.next();
        qint64 pid = s.toLongLong();
        if (pid > 0) {
            ret << pid;
        }
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
