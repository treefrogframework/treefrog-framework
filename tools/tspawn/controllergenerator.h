/* Copyright (c) 2011, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#ifndef CONTROLLERGENERATOR_H
#define CONTROLLERGENERATOR_H

#include <QString>
#include <QDir>


class ControllerGenerator
{
public:
    ControllerGenerator(const QString &controller, const QString &table, const QStringList &actions, const QString &dst);
    ~ControllerGenerator() { }
    bool generate() const;

private:
    QString controllerName;
    QString tableName;
    QStringList actionList;
    QDir dstDir;
};

#endif // CONTROLLERGENERATOR_H
