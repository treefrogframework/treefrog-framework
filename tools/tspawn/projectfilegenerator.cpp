/* Copyright (c) 2010-2012, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QtCore>
#include "projectfilegenerator.h"
#include "filewriter.h"


bool ProjectFileGenerator::exists() const
{
    QFileInfo fi(filePath);
    return fi.exists();
}


bool ProjectFileGenerator::add(const QStringList &files)
{
    QString output;

    QFile pro(filePath);
    if (!pro.exists()) {
        qCritical("Project file not found: %s", qPrintable(pro.fileName()));
        return false;
    } else {
        if (pro.open(QIODevice::ReadOnly | QIODevice::Text)) {
            output = pro.readAll();
        } else {
            qCritical("failed to open file: %s", qPrintable(pro.fileName()));
            return false;
        }
    }
    pro.close();

    for (QStringListIterator i(files); i.hasNext(); ) {
        QString f = QFileInfo(i.next()).fileName();
        QString str;
        QRegExp rx("*.h");
        rx.setPatternSyntax(QRegExp::Wildcard);
        if (rx.exactMatch(f)) {
            str  = "HEADERS += ";
            str += f;
            str += '\n';
        } else {
            rx.setPattern("*.cpp");
            if (rx.exactMatch(f)) {
                str  = "SOURCES += ";
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


bool ProjectFileGenerator::remove(const QStringList &files)
{
    QString output;

    QFile pro(filePath);
    if (pro.exists()) {
        if (pro.open(QIODevice::ReadOnly | QIODevice::Text)) {
            output = pro.readAll();
        } else {
            qCritical("failed to open file: %s", qPrintable(pro.fileName()));
            return false;
        }
    } else {
        qCritical("Project file not found: %s", qPrintable(pro.fileName()));
        return false;
    }
    pro.close();

    if (files.isEmpty())
        return true;

    for (QStringListIterator i(files); i.hasNext(); ) {
        QString f = QFileInfo(i.next()).fileName();
        QString str;
        QRegExp rx("*.h");
        rx.setPatternSyntax(QRegExp::Wildcard);
        if (rx.exactMatch(f)) {
            str  = "HEADERS += ";
            str += f;
            str += '\n';
        } else {
            rx.setPattern("*.cpp");
            if (rx.exactMatch(f)) {
                str  = "SOURCES += ";
                str += f;
                str += '\n';
            }
        }
        
        int idx = 0;
        if (!str.isEmpty() && (idx = output.indexOf(str)) >= 0 ) {
            output.remove(idx, str.length());
        }
    }

    return FileWriter(filePath).write(output, true);
}
