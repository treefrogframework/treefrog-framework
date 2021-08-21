/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "erbparser.h"
#include "erbconverter.h"
#include <THtmlParser>


static QString semicolonTrim(const QString &str)
{
    QString res = str;
    for (;;) {
        res = res.trimmed();
        if (!res.endsWith(';'))
            break;

        res.chop(1);
    }
    return res;
}


static bool isAsciiString(const QString &str)
{
    for (auto &c : str) {
        if (c.unicode() >= 128)
            return false;
    }
    return true;
}


void ErbParser::parse(const QString &erb)
{
    srcCode.clear();
    srcCode.reserve(erb.length() * 2);

    // trimming strongly
    if (trimMode == StrongTrim) {
        erbData.clear();
        for (auto &line : (const QStringList &)erb.split('\n', Tf::SkipEmptyParts)) {
            QString trm = THtmlParser::trim(line);
            if (!trm.isEmpty()) {
                erbData += trm;
                erbData += '\n';
            }
        }
        erbData = THtmlParser::trim(erbData);
    } else {
        erbData = erb;
    }
    pos = 0;

    while (pos < erbData.length()) {
        int i = erbData.indexOf("<%", pos);
        QString text = erbData.mid(pos, i - pos);
        if (!text.isEmpty()) {
            // HTML output
            if (isAsciiString(text)) {
                srcCode += QLatin1String("  responsebody += QStringLiteral(\"");
            } else {
                srcCode += QLatin1String("  responsebody += tr(\"");
            }
            srcCode += ErbConverter::escapeNewline(text);
            srcCode += QLatin1String("\");\n");
        }

        if (i >= 0) {
            pos = i;
            parsePercentTag();
        } else {
            break;
        }
    }
}


bool ErbParser::posMatchWith(const QString &str, int offset) const
{
    return (pos + offset >= 0 && pos + offset + str.length() - 1 < erbData.length()
        && erbData.mid(pos + offset, str.length()) == str);
}


void ErbParser::parsePercentTag()
{
    if (!posMatchWith("<%")) {
        return;
    }
    pos += 2;
    startTag = "<%";

    srcCode += QLatin1String("  ");  // Appends indent
    QString str;
    QChar c = erbData[pos++];
    if (c == QLatin1Char('#')) {  // <%#
        startTag += c;
        if (posMatchWith("include ") || posMatchWith("include\t")) {
            startTag += QLatin1String("include");
            // Outputs include-macro
            incCode += QLatin1Char('#');
            QPair<QString, QString> p = parseEndPercentTag();
            incCode += p.first;
            incCode += QLatin1Char('\n');
        } else {
            // Outputs comments
            srcCode += QLatin1String("/*");
            QPair<QString, QString> p = parseEndPercentTag();
            srcCode += p.first;
            srcCode += QLatin1String("*/\n");
        }

    } else if (c == QLatin1Char('=')) {
        startTag += c;
        if (posMatchWith("=$")) {  // <%==$
            startTag += QLatin1String("=$");
            pos += 2;
            // Outputs 'echo' the export value
            QPair<QString, QString> p = parseEndPercentTag();
            if (!p.first.isEmpty()) {
                if (p.second.isEmpty()) {
                    srcCode += QLatin1String("techoex(");
                    srcCode += semicolonTrim(p.first);
                    srcCode += QLatin1String(");\n");
                } else {
                    srcCode += QLatin1String("techoex2(");
                    srcCode += semicolonTrim(p.first);
                    srcCode += QLatin1String(", (");
                    srcCode += semicolonTrim(p.second);
                    srcCode += QLatin1String("));\n");
                }
            }

        } else if (posMatchWith("$")) {  // <%=$
            startTag += "$";
            ++pos;
            // Outputs 'eh' the export value
            QPair<QString, QString> p = parseEndPercentTag();
            if (!p.first.isEmpty()) {
                if (p.second.isEmpty()) {
                    srcCode += QLatin1String("tehex(");
                    srcCode += semicolonTrim(p.first);
                    srcCode += QLatin1String(");\n");
                } else {
                    srcCode += QLatin1String("tehex2(");
                    srcCode += semicolonTrim(p.first);
                    srcCode += QLatin1String(", (");
                    srcCode += semicolonTrim(p.second);
                    srcCode += QLatin1String("));\n");
                }
            }

        } else if (posMatchWith("=")) {  // <%==
            startTag += QLatin1Char('=');
            ++pos;
            // Outputs the value
            QPair<QString, QString> p = parseEndPercentTag();
            if (!p.first.isEmpty()) {
                if (p.second.isEmpty()) {
                    srcCode += QLatin1String("responsebody += QVariant(");
                    srcCode += semicolonTrim(p.first);
                    srcCode += QLatin1String(").toString();\n");
                } else {
                    srcCode += QLatin1String("{ QString ___s = QVariant(");
                    srcCode += semicolonTrim(p.first);
                    srcCode += QLatin1String(").toString(); responsebody += (___s.isEmpty()) ? QVariant(");
                    srcCode += semicolonTrim(p.second);
                    srcCode += QLatin1String(").toString() : ___s; }\n");
                }
            }

        } else {  // <%=
            // Outputs the escaped value
            QPair<QString, QString> p = parseEndPercentTag();
            if (!p.first.isEmpty()) {
                if (p.second.isEmpty()) {
                    srcCode += QLatin1String("responsebody += THttpUtility::htmlEscape(");
                    srcCode += semicolonTrim(p.first);
                    srcCode += QLatin1String(");\n");
                } else {
                    srcCode += QLatin1String("{ QString ___s = QVariant(");
                    srcCode += semicolonTrim(p.first);
                    srcCode += QLatin1String(").toString(); responsebody += (___s.isEmpty()) ? THttpUtility::htmlEscape(");
                    srcCode += semicolonTrim(p.second);
                    srcCode += QLatin1String(") : THttpUtility::htmlEscape(___s); }\n");
                }
            }
        }

    } else {  // <%
        --pos;
        QPair<QString, QString> p = parseEndPercentTag();
        str = p.first.trimmed();
        if (!str.endsWith(';') && !str.endsWith('{')) {
            str += QLatin1Char(';');
        }
        // Raw codes
        srcCode += str;
        srcCode += QLatin1Char('\n');
    }
}


QPair<QString, QString> ErbParser::parseEndPercentTag()
{
    QString string;
    QString defaultVal;
    bool defaultFlag = false;

    while (pos < erbData.length()) {
        if (posMatchWith("%>")) {
            pos += 2;
            QChar c = erbData[pos - 3];
            if (c == QLatin1Char('-') || c == QLatin1Char('+')) {  // -%> or +%>
                if (defaultFlag) {
                    defaultVal.chop(1);
                } else {
                    string.chop(1);
                }

                if (c == QLatin1Char('-'))
                    skipWhiteSpacesAndNewLineCode();

            } else if (trimMode == NormalTrim || trimMode == StrongTrim) {  // NormalTrim:1
                if (startTag == QLatin1String("<%") || startTag.startsWith("<%#")) {
                    skipWhiteSpacesAndNewLineCode();
                }

            } else if (trimMode == TrimOff) {  // TrimOff:0
                // do not skip whitespaces
            } else {
                qCritical("Invalid arguments: trim mode: %d", trimMode);
            }
            break;
        }

        if (posMatchWith("%|%")) {
            defaultFlag = true;
            pos += 3;
        }

        QChar c = erbData[pos];
        if (c == QLatin1Char('\'') || c == QLatin1Char('"')) {
            if (defaultFlag) {
                defaultVal += parseQuote();
            } else {
                string += parseQuote();
            }
        } else {
            if (defaultFlag) {
                defaultVal += c;
            } else {
                string += c;
            }
            ++pos;
        }
    }
    return qMakePair(string, defaultVal.trimmed());
}


void ErbParser::skipWhiteSpacesAndNewLineCode()
{
    // Skip white-spaces and new line code
    int p = pos;
    while (pos < erbData.length()) {
        QChar c = erbData[pos++];
        if (c == QLatin1Char('\n')) {
            break;
        }

        if (!c.isSpace() || c.unicode() >= 128) {
            pos = p;  // no skip
            break;
        }
    }
}


QString ErbParser::parseQuote()
{
    QChar m = erbData[pos];  // first quote
    if (m != QLatin1Char('\'') && m != QLatin1Char('"')) {
        return QString();
    }
    ++pos;

    QString quote = m;
    while (pos < erbData.length()) {
        quote += erbData[pos++];
        if (erbData[pos - 1] == m && erbData[pos - 2] != QLatin1Char('\\')) {
            break;
        }
    }
    return quote;
}
