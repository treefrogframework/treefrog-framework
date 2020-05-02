/* Copyright (c) 2011-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tprocessinfo.h"
#include <QtCore>
#include <signal.h>
#include <sys/types.h>


bool TProcessInfo::exists() const
{
    return QFileInfo(QLatin1String("/proc/") + QString::number(processId) + "/status").exists();
}


qint64 TProcessInfo::ppid() const
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


QString TProcessInfo::processName() const
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


QList<qint64> TProcessInfo::allConcurrentPids()
{
    QList<qint64> ret;
    QDir proc("/proc");
    const QStringList dirs = proc.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);

    for (const auto &s : dirs) {
        qint64 pid = s.toLongLong();
        if (pid > 0) {
            ret << pid;
        }
    }

    std::sort(ret.begin(), ret.end());  // Sorts the items
    return ret;
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
