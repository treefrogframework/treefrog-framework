/* Copyright (c) 2011, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#ifndef MODELGENERATOR_H
#define MODELGENERATOR_H

#include <QStringList>
#include <QDir>
#include <QPair>


class ModelGenerator
{
public:
    ModelGenerator(const QString &model, const QString &table, const QStringList &fields, const QString &dst);
    bool generate(bool userModel = false) const;
    QString generateSqlObject() const;
    QString model() const { return modelName; }

protected:
    QStringList genModel() const;
    QStringList genUserModel(const QString &usernameField = "username", const QString &passwordField = "password") const;
    QPair<QStringList, QStringList> createModelParams() const;
    static bool gen(const QString &fileName, const QString &format, const QStringList &args);
    static QString createParam(const QString &type, const QString &name);

private:
    QString modelName;
    QString tableName;
    QDir dstDir;
    QStringList fieldList;
};

#endif // MODELGENERATOR_H
