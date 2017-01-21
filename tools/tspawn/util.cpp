/* Copyright (c) 2011-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QDir>
#include <QFileInfo>
#include <cstdio>
#include "util.h"

QString dataDirPath = QLatin1String(TREEFROG_DATA_DIR) + QDir::separator() + "defaults" + QDir::separator();


void copy(const QString &src, const QString &dst)
{
    QFile sfile(src);
    QFileInfo dfile(dst);
    if (sfile.exists() && !dfile.exists()) {
        if (sfile.copy(dst)) {
            printf("  created   %s\n", qPrintable(QDir::cleanPath(dst)));
        } else {
            qCritical("failed to create a file %s", qPrintable(QDir::cleanPath(dst)));
        }
    }
}


void copy(const QString &src, const QDir &dst)
{
    QFileInfo fi(src);
    copy(src, dst.filePath(fi.fileName()));
}


bool rmpath(const QString &path)
{
    bool res = QDir(path).rmpath(".");
    if (res) {
        printf("  removed   %s\n", qPrintable(QDir::cleanPath(path)));
    }
    return res;
}


bool remove(const QString &file)
{
    QFile f(file);
    return remove(f);
}


bool remove(QFile &file)
{
    bool ret = file.remove();
    if (ret) {
        printf("  removed   %s\n", qPrintable(QDir::cleanPath(file.fileName())));
    }
    return ret;
}


bool replaceString(const QString &fileName, const QByteArray &before, const QByteArray &after)
{
    QFile file(fileName);
    if (!file.exists() || !file.open(QIODevice::ReadOnly))
        return false;

    QByteArray text = file.readAll();
    text.replace(before, after);
    file.close();

    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;

    file.write(text);
    file.close();
    return true;
}
