/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QFile>
#include <QDir>
#include <QDataStream>
#include <QReadWriteLock>
#include <TWebApplication>
#include "tsessionfilestore.h"
#include "tsystemglobal.h"
#include "tfcore.h"

#define SESSION_DIR_NAME "session"

static QReadWriteLock rwLock;  // Global read-write lock

/*!
  \class TSessionFileStore
  \brief The TSessionFileStore class stores HTTP sessions to files.
*/

bool TSessionFileStore::store(TSession &session)
{
    QDir dir(sessionDirPath());
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // perform long-lasting operations before blocking threads and proceses
    QByteArray buffer;
    QDataStream ds(&buffer, QIODevice::WriteOnly);
    ds << *static_cast<const QVariantMap *>(&session);
    if (ds.status() != QDataStream::Ok) {
        tSystemError("Failed to store session. Must set objects that can be serialized.");
        return false;
    }
    buffer = Tf::lz4Compress(buffer);  // compress

    QWriteLocker locker(&rwLock);  // lock for threads
    QFile file(sessionDirPath() + session.id());
    if (!file.open(QIODevice::ReadWrite)) {
        tSystemError("Failed to store session. File open error.");
        return false;
    }
    if (Tf::app()->maxNumberOfAppServers() > 1) {
        auto reslock = tf_lockfile(file.handle(), true, true);  // blocking flock for processes
        if (reslock < 0) {
            int err = errno;
            tSystemWarn("flock error  errno:%d", err);
        }
    }
    file.resize(0);  // truncate
    if (file.write(buffer) == -1) {
        tSystemError("Failed to store session. File write error.");
        return false;
    }
    return true;
}


TSession TSessionFileStore::find(const QByteArray &id)
{
    QFileInfo fi(sessionDirPath() + id);
    QDateTime expire = QDateTime::currentDateTime().addSecs(-lifeTimeSecs());

    if (!fi.exists() || fi.lastModified() < expire) {
        return TSession();
    }

    QByteArray buffer;
    {
        QReadLocker locker(&rwLock);  // lock for threads
        QFile file(fi.filePath());
        if (!file.open(QIODevice::ReadOnly)) {
            tSystemError("Failed to load a session from the file store.");
            return TSession();
        }
        if (Tf::app()->maxNumberOfAppServers() > 1) {
            auto reslock = tf_lockfile(file.handle(), false, true);  // blocking flock for processes
            if (reslock < 0) {
                int err = errno;
                tSystemWarn("flock error  errno:%d", err);
            }
        }
        buffer = file.readAll();
    } // release all locks

    buffer = Tf::lz4Uncompress(buffer);  // uncompress
    if (buffer.isEmpty()) {
        tSystemError("Failed to load a session from the file store.");
        return TSession();
    }

    TSession session(id);
    QDataStream ds(&buffer, QIODevice::ReadOnly);
    ds >> *static_cast<QVariantMap *>(&session);
    if (ds.status() != QDataStream::Ok) {
        tSystemError("Failed to load a session from the file store.");
        return TSession();
    }
    return session;
}


bool TSessionFileStore::remove(const QByteArray &id)
{
    return QFile::remove(sessionDirPath() + id);
}


int TSessionFileStore::gc(const QDateTime &expire)
{
    int res = 0;
    QDir dir(sessionDirPath());
    if (dir.exists()) {
        const QList<QFileInfo> lst = dir.entryInfoList(QDir::Files, QDir::Time | QDir::Reversed);
        for (auto &fi : lst) {
            if (fi.lastModified() < expire) {
                if (dir.remove(fi.fileName())) {
                    res++;
                }
            } else {
                break;
            }
        }
    }
    return res;
}


QString TSessionFileStore::sessionDirPath()
{
    QString path;
    path.reserve(256);
    path += Tf::app()->tmpPath();
    path += QLatin1String(SESSION_DIR_NAME);
    path += QDir::separator();
    return path;
}
