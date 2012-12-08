/* Copyright (c) 2011, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#ifndef MAILERGENERATOR_H
#define MAILERGENERATOR_H

#include <QString>
#include <QDir>


class MailerGenerator
{
public:
    MailerGenerator(const QString &name, const QStringList &actions, const QString &dst);
    bool generate() const;

private:
    QString mailerName;
    QStringList actionList;
    QDir dstDir;
};

#endif // MAILERGENERATOR_H
