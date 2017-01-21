/* Copyright (c) 2010-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QSet>
#include <QRegExp>
#include <QStringList>
#include <tabstractmodel.h>
#include "global.h"

class UpperWords : public QSet<QString>
{
public:
    UpperWords() : QSet<QString>()
    {
        insert("id");
    }
};
Q_GLOBAL_STATIC(UpperWords, upperWords)


class LowerWords : public QSet<QString>
{
public:
    LowerWords() : QSet<QString>()
    {
        insert("about");
        insert("above");
        insert("across");
        insert("after");
        insert("against");
        insert("along");
        insert("among");
        insert("and");
        insert("as");
        insert("at");
        insert("before");
        insert("behind");
        insert("below");
        insert("beside");
        insert("besides");
        insert("between");
        insert("beyond");
        insert("by");
        insert("down");
        insert("during");
        insert("except");
        insert("for");
        insert("from");
        insert("in");
        insert("into");
        insert("of");
        insert("off");
        insert("on");
        insert("onto");
        insert("or");
        insert("out");
        insert("over");
        insert("since");
        insert("than");
        insert("through");
        insert("till");
        insert("to");
        insert("toward");
        insert("under");
        insert("until");
        insert("up");
        insert("upon");
        insert("via");
        insert("with");
        insert("within");
        insert("without");
    }
};
Q_GLOBAL_STATIC(LowerWords, lowerWords)


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
    QStringList caplist = cap.split(QRegExp("\\W+"), QString::SkipEmptyParts);
    for (QMutableStringListIterator i(caplist); i.hasNext(); ) {
        QString &s = i.next();
        QString slow = s.toLower();
        if (upperWords()->contains(slow)) {
            s = s.toUpper();
        } else if (lowerWords()->contains(slow)) {
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
            qCritical("failed to create a directory %s", qPrintable(QDir::cleanPath(dir.filePath(dirPath))));
            return false;
        }
        printf("  created   %s\n", qPrintable(QDir::cleanPath(dir.filePath(dirPath))));
    }
    return true;
}
