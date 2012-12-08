/* Copyright (c) 2011, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#ifndef GLOBAL_H
#define GLOBAL_H

#include <QString>
#include <QDir>

QString fieldNameToVariableName(const QString &name);
QString variableNameToFieldName(const QString &name);

QString fieldNameToEnumName(const QString &name);
QString enumNameToFieldName(const QString &name);

QString enumNameToVariableName(const QString &name);
QString variableNameToEnumName(const QString &name);

QString fieldNameToCaption(const QString &name);
QString enumNameToCaption(const QString &name);

bool mkpath(const QDir &dir, const QString &dirPath = ".");

#endif // GLOBAL_H
