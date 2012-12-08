/* Copyright (c) 2011, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#ifndef OTAMAGENERATOR_H
#define OTAMAGENERATOR_H

#include <QStringList>
#include <QDir>


class OtamaGenerator
{
public:
    OtamaGenerator(const QString &view, const QString &table, const QString &dst);
    bool generate() const;

protected:
    QStringList generateViews() const;

private:
    QString viewName;
    QString tableName;
    QDir dstDir;
};

#endif // OTAMAGENERATOR_H
