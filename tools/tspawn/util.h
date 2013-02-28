/* Copyright (c) 2012, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#ifndef UTIL_H
#define UTIL_H

#include <QString>
#include <QDir>
#include <QByteArray>

extern QString dataDirPath;

extern void copy(const QString &src, const QString &dst);
extern void copy(const QString &src, const QDir &dst);
extern bool rmpath(const QString &path);
extern bool remove(QFile &file);
extern bool replaceString(const QString &fileName, const QByteArray &before, const QByteArray &after);

#endif // UTIL_H
