/* Copyright (c) 2012, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#ifndef ERBGENERATOR_H
#define ERBGENERATOR_H

#include <QStringList>
#include <QDir>


class ErbGenerator
{
public:
    ErbGenerator(const QString &view, const QString &table, const QString &dst);
    bool generate() const;

private:
    QString viewName;
    QString tableName;
    QDir dstDir;
};

#endif // ERBGENERATOR_H
