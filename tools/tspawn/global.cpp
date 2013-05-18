/* Copyright (c) 2010-2012, AOYAMA Kazuharu
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

Q_GLOBAL_STATIC_WITH_INITIALIZER(QSet<QString>, upperWords,
{
    x->insert("id");
})

Q_GLOBAL_STATIC_WITH_INITIALIZER(QSet<QString>, lowerWords,
{
    x->insert("about");
    x->insert("above");
    x->insert("across");
    x->insert("after");
    x->insert("against");
    x->insert("along");
    x->insert("among");
    x->insert("and");
    x->insert("as");
    x->insert("at");
    x->insert("before");
    x->insert("behind");
    x->insert("below");
    x->insert("beside");
    x->insert("besides");
    x->insert("between");
    x->insert("beyond");
    x->insert("by");
    x->insert("down");
    x->insert("during");
    x->insert("except");
    x->insert("for");
    x->insert("from");
    x->insert("in");
    x->insert("into");
    x->insert("of");
    x->insert("off");
    x->insert("on");
    x->insert("onto");
    x->insert("or");
    x->insert("out");
    x->insert("over");
    x->insert("since");
    x->insert("than");
    x->insert("through");
    x->insert("till");
    x->insert("to");
    x->insert("toward");
    x->insert("under");
    x->insert("until");
    x->insert("up");
    x->insert("upon");
    x->insert("via");
    x->insert("with");
    x->insert("within");
    x->insert("without");
})


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
