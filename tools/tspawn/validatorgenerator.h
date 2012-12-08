/* Copyright (c) 2011, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#ifndef VALIDATORGENERATOR_H
#define VALIDATORGENERATOR_H

#include <QtCore>


class ValidatorGenerator
{
public:
    ValidatorGenerator(const QString &validator, const QString &dst);
    bool generate() const;

private:
    QString name;
    QDir dstDir;
};

#endif // VALIDATORGENERATOR_H

