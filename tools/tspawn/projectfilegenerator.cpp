/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "projectfilegenerator.h"
#include "filewriter.h"
#include <QtCore>


bool ProjectFileGenerator::exists() const
{
    QFileInfo fi(filePath);
    return fi.exists();
}


bool ProjectFileGenerator::add(const QStringList &files) const
{
    static const QRegularExpression reh(".+\\.h$");
    static const QRegularExpression recpp(".+\\.cpp$");

    QString output;

    QFile pro(filePath);
    if (!pro.exists()) {
        qCritical("Project file not found: %s", qUtf8Printable(pro.fileName()));
        return false;
    } else {
        if (pro.open(QIODevice::ReadOnly | QIODevice::Text)) {
            output = pro.readAll();
        } else {
            qCritical("failed to open file: %s", qUtf8Printable(pro.fileName()));
            return false;
        }
    }
    pro.close();

    QString str;
    for (QStringListIterator i(files); i.hasNext();) {
        const QString &f = i.next();

        auto match = reh.match(f);
        if (match.hasMatch()) {
            str = "HEADERS += ";
            str += f;
            str += '\n';
        } else {
            match = recpp.match(f);
            if (match.hasMatch()) {
                str = "SOURCES += ";
                str += f;
                str += '\n';
            }
        }

        if (!str.isEmpty() && !output.contains(str)) {
            if (!output.endsWith('\n')) {
                output += '\n';
            }
            output += str;
        }
    }

    return FileWriter(filePath).write(output, true);
}


bool ProjectFileGenerator::remove(const QStringList &files) const
{
    static const QRegularExpression reh(".+\\.h$");
    static const QRegularExpression recpp(".+\\.cpp$");

    QString output;

    QFile pro(filePath);
    if (pro.exists()) {
        if (pro.open(QIODevice::ReadOnly | QIODevice::Text)) {
            output = pro.readAll();
            pro.close();
        } else {
            qCritical("failed to open file: %s", qUtf8Printable(pro.fileName()));
            return false;
        }
    } else {
        qCritical("Project file not found: %s", qUtf8Printable(pro.fileName()));
        return false;
    }

    if (files.isEmpty())
        return true;

    QString str;
    for (const QString &f : files) {
        auto match = reh.match(f);
        if (match.hasMatch()) {
            str = "HEADERS += ";
            str += f;
        } else {
            match = recpp.match(f);
            if (match.hasMatch()) {
                str = "SOURCES += ";
                str += f;
            }
        }

#ifdef Q_OS_WIN
        str.replace("\\", "/");
#endif

        int idx = 0;
        if (!str.isEmpty() && (idx = output.indexOf(str)) >= 0) {
            output.remove(idx, str.length());
            if (idx < output.length() && output.at(idx) == '\n') {
                output.remove(idx, 1);
            }
        }
    }

    return FileWriter(filePath).write(output, true);
}
