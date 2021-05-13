/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "otmparser.h"
#include <TGlobal>
#include <QHash>
#include <QRegExp>
#include <QTextStream>

#define INCLUDE_LABEL QLatin1String("#include")
#define INIT_LABEL QLatin1String("#init")
#define NORMAL_ECHO QString("==")
#define ESCAPE_ECHO QString("=")
#define EXVAR_ECHO QString("==$")
#define EXVAR_ESCAPE_ECHO QString("=$")


class OperatorHash : public QHash<int, QString> {
public:
    OperatorHash() :
        QHash<int, QString>()
    {
        insert(OtmParser::TagReplacement, ":");
        insert(OtmParser::ContentAssignment, "~");
        insert(OtmParser::AttributeSet, "+");
        insert(OtmParser::TagMerging, "|==");
    }
};
Q_GLOBAL_STATIC(OperatorHash, opHash)


OtmParser::OtmParser(const QString &replaceMarker) :
    repMarker(replaceMarker)
{
}


void OtmParser::parse(const QString &text)
{
    entries.clear();

    QString txt = text;
    QTextStream ts(&txt, QIODevice::ReadOnly);

    bool lineCont = false;
    QString line, label, value;
    for (;;) {
        line = ts.readLine();
        if (line.trimmed().isEmpty()) {
            lineCont = false;
            if (!label.isEmpty()) {
                QString str = value.trimmed();
                QRegExp rx("[^:~\\+\\|].*");  // anything but ':', '~', '+', '|'
                if (rx.indexIn(str) == 0) {
                    // Regard empty Otama operator as the ':' operator
                    str = QLatin1Char(':') + str;
                }

                entries.insert(label, str);
                label.clear();
                value.clear();
            }

        } else if (lineCont) {
            // line continue
            value += QLatin1Char('\n');
            value += line;

        } else if ((line.startsWith('#') || line.startsWith('@')) && line.length() > 1
            && !line.at(1).isSpace()) {
            // search the end of label
            int i = line.indexOf(QRegularExpression("[^a-zA-Z0-9_]"), 1);

            if (line.startsWith(INCLUDE_LABEL + QLatin1Char(' '))) {
                entries.insert(line.left(i), line.mid(i).trimmed());
            } else {
                // New label
                lineCont = true;
                if (i > 0) {
                    label = line.left(i);
                    value = line.mid(i);
                } else {
                    label = line;
                }
            }

        } else {
            // Ignore the line
        }

        if (line.isNull()) {
            // EOF
            break;
        }
    }
}


QString OtmParser::getSrcCode(const QString &label, OperatorType op, EchoOption *option) const
{
    QString code;
    EchoOption opt = None;

    QStringList lst = entries.values(label);
    for (QListIterator<QString> i(lst); i.hasNext();) {
        const QString &s = i.next();
        QString opstr = opHash()->value(op);  // Gets operator string

        if (!opstr.isEmpty() && s.startsWith(opstr) && !s.contains(repMarker)) {
            code = s.mid(opstr.length());

            if (op != TagMerging) {
                Q_ASSERT(EXVAR_ECHO.length() >= EXVAR_ESCAPE_ECHO.length());
                Q_ASSERT(NORMAL_ECHO.length() >= ESCAPE_ECHO.length());

                if (code.startsWith(EXVAR_ECHO)) {
                    opt = ExportVarEcho;
                    code.remove(0, EXVAR_ECHO.length());
                } else if (code.startsWith(EXVAR_ESCAPE_ECHO)) {
                    opt = ExportVarEscapeEcho;
                    code.remove(0, EXVAR_ESCAPE_ECHO.length());
                } else if (code.startsWith(NORMAL_ECHO)) {
                    opt = NormalEcho;
                    code.remove(0, NORMAL_ECHO.length());
                } else if (code.startsWith(ESCAPE_ECHO)) {
                    opt = EscapeEcho;
                    code.remove(0, ESCAPE_ECHO.length());
                }
            } else {
                if (code.startsWith('$')) {  // |==$ operator
                    opt = ExportVarEcho;
                    code.remove(0, 1);
                }
            }
            code = code.trimmed();
            break;
        }
    }

    if (option)
        *option = opt;

    return code;
}


QStringList OtmParser::getWrapSrcCode(const QString &label, OperatorType op) const
{
    if (op == TagReplacement || op == ContentAssignment) {
        QStringList lst = entries.values(label);
        QListIterator<QString> i(lst);
        while (i.hasNext()) {
            const QString &s = i.next();
            QString opstr = opHash()->value(op);

            if (!opstr.isEmpty() && s.startsWith(opstr) && s.contains(repMarker)) {
                return s.mid(opstr.length()).trimmed().split(repMarker, Tf::SkipEmptyParts, Qt::CaseSensitive);
            }
        }
    }
    return QStringList();
}


QStringList OtmParser::includeStrings() const
{
    QStringList res;
    QStringList lst = entries.values(INCLUDE_LABEL);
    QListIterator<QString> i(lst);
    while (i.hasNext()) {
        const QString &s = i.next();
        res << INCLUDE_LABEL + QLatin1Char(' ') + s;
    }
    return res;
}


QString OtmParser::getInitSrcCode() const
{
    QString code = entries.value(INIT_LABEL);

    if (!code.isEmpty()) {
        QRegularExpression rx(QLatin1String("^[") + opHash()->value(TagReplacement) + opHash()->value(ContentAssignment) + opHash()->value(AttributeSet) + "]={0,2}");

        if (code.contains(rx)) {
            code.remove(rx);
        } else if (code.startsWith(opHash()->value(TagMerging))) {
            code.remove(0, opHash()->value(TagMerging).length());
        }
    }

    return code.trimmed();
}
