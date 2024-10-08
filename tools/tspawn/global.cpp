/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "global.h"
#include <QRegularExpression>
#include <QSet>
#include <QStringList>
#include <tabstractmodel.h>

const QSet<QString> upperWords = { "id" };

const QSet<QString> lowerWords = {
    {"about"},
    {"above"},
    {"across"},
    {"after"},
    {"against"},
    {"along"},
    {"among"},
    {"and"},
    {"as"},
    {"at"},
    {"before"},
    {"behind"},
    {"below"},
    {"beside"},
    {"besides"},
    {"between"},
    {"beyond"},
    {"by"},
    {"down"},
    {"during"},
    {"except"},
    {"for"},
    {"from"},
    {"in"},
    {"into"},
    {"of"},
    {"off"},
    {"on"},
    {"onto"},
    {"or"},
    {"out"},
    {"over"},
    {"since"},
    {"than"},
    {"through"},
    {"till"},
    {"to"},
    {"toward"},
    {"under"},
    {"until"},
    {"up"},
    {"upon"},
    {"via"},
    {"with"},
    {"within"},
    {"without"},
};


QString fieldNameToVariableName(const QString &name)
{
    return TAbstractModel::fieldNameToVariableName(name);
}


QString fieldNameToEnumName(const QString &name)
{
    QString obj = fieldNameToVariableName(name);
    if (!obj.isEmpty()) {
        obj[0] = obj[0].toUpper();
    }
    return obj;
}


static QString enumNameToFieldName(const QString &name)
{
    QString str;
    for (int i = 0; i < name.length(); ++i) {
        if (name[i].isUpper()) {
            if (i > 0) {
                str += '_';
            }
            str += name[i].toLower();
        } else {
            str += name[i];
        }
    }
    return str;
}


QString enumNameToVariableName(const QString &name)
{
    QString var = name;
    if (!var.isEmpty()) {
        var[0] = var[0].toLower();
    }
    return var;
}


QString fieldNameToCaption(const QString &name)
{
    QString cap;
    for (int i = 0; i < name.length(); ++i) {
        if (name[i] == '_') {
            cap += ' ';
        } else {
            if (i > 0 && name[i - 1] == '_') {
                cap += name[i].toUpper();
            } else {
                cap += name[i];
            }
        }
    }

    // Upper-case/lower-case words
    QStringList caplist = cap.split(QRegularExpression("\\W+"), Tf::SkipEmptyParts);
    for (QMutableStringListIterator i(caplist); i.hasNext();) {
        QString &s = i.next();
        QString slow = s.toLower();
        if (upperWords.contains(slow)) {
            s = s.toUpper();
        } else if (lowerWords.contains(slow)) {
            s = slow;
        }
    }
    cap = caplist.join(" ");

    if (!cap.isEmpty()) {
        cap[0] = cap[0].toUpper();
    }
    return cap;
}


QString enumNameToCaption(const QString &name)
{
    return fieldNameToCaption(enumNameToFieldName(name));
}


bool mkpath(const QDir &dir, const QString &dirPath)
{
    if (!dir.exists(dirPath)) {
        if (!dir.mkpath(dirPath)) {
            qCritical("failed to create a directory %s", qUtf8Printable(QDir::cleanPath(dir.filePath(dirPath))));
            return false;
        }
        std::printf("  created   %s\n", qUtf8Printable(QDir::cleanPath(dir.filePath(dirPath))));
    }
    return true;
}


QString replaceholder(const QString &format, const QPair<QString, QString> &value)
{
    QString out = format;
    QString placeholder = QLatin1Char('%') + value.first + QLatin1Char('%');
    out.replace(placeholder, value.second);
    return out;
}


QString replaceholder(const QString &format, const PlaceholderList &values)
{
    QString out = format;
    for (auto &p : values) {
        out = replaceholder(out, p);
    }
    return out;
}
