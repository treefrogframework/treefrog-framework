/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "otamaconverter.h"
#include "otmparser.h"
#include "viewconverter.h"
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QTextStream>
#include <THtmlParser>

#define TF_ATTRIBUTE_NAME QLatin1String("data-tf")
#define LEFT_DELIM QString("<% ")
#define RIGHT_DELIM QString(" %>")
#define RIGHT_DELIM_NO_TRIM QString(" +%>")
#define DUMMY_LABEL QLatin1String("@dummy")
#define DUMMYTAG_LABEL QLatin1String("@dummytag")

QString devIni;
static QString replaceMarker;


QString generateErbPhrase(const QString &str, int echoOption)
{
    QString s = str;
    s.remove(QRegularExpression(";+$"));
    QString res = LEFT_DELIM;

    if (echoOption == OtmParser::None) {
        res += s;
        res += RIGHT_DELIM;
    } else {
        switch (echoOption) {
        case OtmParser::NormalEcho:
            res += QLatin1String("echo(");
            break;

        case OtmParser::EscapeEcho:
            res += QLatin1String("eh(");
            break;

        case OtmParser::ExportVarEcho:
            res += QLatin1String("techoex(");
            break;

        case OtmParser::ExportVarEscapeEcho:
            res += QLatin1String("tehex(");
            break;

        default:
            qCritical("Internal error, bad option: %d", echoOption);
            return QString();
            break;
        }

        res += s;
        res += ')';
        res += RIGHT_DELIM_NO_TRIM;
    }

    return res;
}


OtamaConverter::OtamaConverter(const QDir &output, const QDir &helpers, const QDir &partial) :
    erbConverter(output, helpers, partial)
{
}


OtamaConverter::~OtamaConverter()
{
}


bool OtamaConverter::convert(const QString &filePath, int trimMode) const
{
    QFile htmlFile(filePath);
    QFile otmFile(ViewConverter::changeFileExtension(filePath, logicFileSuffix()));
    QString className = ViewConverter::getViewClassName(filePath);
    QFile outFile(erbConverter.outputDir().filePath(className + ".cpp"));

    // Checks file's timestamp
    QFileInfo htmlFileInfo(htmlFile);
    QFileInfo otmFileInfo(otmFile);
    QFileInfo outFileInfo(outFile);
    if (htmlFileInfo.exists() && outFileInfo.exists()) {
        if (outFileInfo.lastModified() > htmlFileInfo.lastModified()
            && (!otmFileInfo.exists() || outFileInfo.lastModified() > otmFileInfo.lastModified())) {
            //std::printf("done    %s\n", qUtf8Printable(outFile.fileName()));
            return true;
        } else {
            if (outFile.remove()) {
                std::printf("  removed  %s\n", qUtf8Printable(outFile.fileName()));
            }
        }
    }

    if (!htmlFile.open(QIODevice::ReadOnly)) {
        qCritical("failed to read phtm file : %s", qUtf8Printable(htmlFile.fileName()));
        return false;
    }

    // Otama logic reading
    QString otm;
    if (otmFile.open(QIODevice::ReadOnly)) {
        otm = QTextStream(&otmFile).readAll();
    }

    QString erb = OtamaConverter::convertToErb(QTextStream(&htmlFile).readAll(), otm, trimMode);
    return erbConverter.convert(className, erb, trimMode);
}


QString OtamaConverter::convertToErb(const QString &html, const QString &otm, int trimMode)
{
    if (replaceMarker.isEmpty()) {
        // Sets a replace-marker
        QSettings devSetting(devIni, QSettings::IniFormat);
        replaceMarker = devSetting.value("Otama.ReplaceMarker", "%%").toString();
    }

    // Parses HTML and Otama files
    THtmlParser htmlParser((THtmlParser::TrimMode)trimMode);
    htmlParser.parse(html);

    OtmParser otmParser(replaceMarker);
    otmParser.parse(otm);

    // Inserts include-header
    QStringList inc = otmParser.includeStrings();
    for (QListIterator<QString> it(inc); it.hasNext();) {
        THtmlElement &e = htmlParser.insertNewElement(0, 0);
        e.text = LEFT_DELIM.trimmed();
        e.text += it.next();
        e.text += RIGHT_DELIM;
    }

    // Inserts init-code
    QString init = otmParser.getInitSrcCode();
    if (!init.isEmpty()) {
        THtmlElement &e = htmlParser.at(0);
        e.text = LEFT_DELIM + init + RIGHT_DELIM + e.text;
    }

    for (int i = htmlParser.elementCount() - 1; i > 0; --i) {
        THtmlElement &e = htmlParser.at(i);
        QString label = e.attribute(TF_ATTRIBUTE_NAME);

        if (e.hasAttribute(TF_ATTRIBUTE_NAME)) {
            e.removeAttribute(TF_ATTRIBUTE_NAME);
        }

        if (label == DUMMY_LABEL) {
            htmlParser.removeElementTree(i, true);
            continue;
        }

        if (label == DUMMYTAG_LABEL) {
            htmlParser.removeTag(i);
            continue;
        }

        QString val;
        OtmParser::EchoOption ech;
        bool scriptArea = htmlParser.parentExists(i, "script");

        // Content assignment
        val = otmParser.getSrcCode(label, OtmParser::ContentAssignment, &ech);  // ~ operator
        if (!val.isEmpty()) {
            htmlParser.removeChildElements(i);
            e.text = generateErbPhrase(val, ech);
        } else {
            QStringList vals = otmParser.getWrapSrcCode(label, OtmParser::ContentAssignment);
            if (!vals.isEmpty()) {
                // Adds block codes
                QString bak = e.text;
                e.text = LEFT_DELIM;
                e.text += vals[0].trimmed();
                e.text += (scriptArea ? RIGHT_DELIM_NO_TRIM : RIGHT_DELIM);
                e.text += bak;
                QString s = vals.value(1).trimmed();
                if (!s.isEmpty()) {
                    THtmlElement &child = htmlParser.appendNewElement(i);
                    child.text = LEFT_DELIM;
                    child.text += s;
                    child.text += (scriptArea ? RIGHT_DELIM_NO_TRIM : RIGHT_DELIM);
                }
            }
        }

        // Tag merging
        val = otmParser.getSrcCode(label, OtmParser::TagMerging, &ech);  // |== operator
        if (!val.isEmpty()) {
            val.remove(QRegularExpression(";+$"));

            QString attr;
            attr = LEFT_DELIM;
            attr += QLatin1String("do { THtmlParser ___pr = THtmlParser::mergeElements(tr(\"");
            attr += ErbConverter::escapeNewline(e.toString());
            attr += QLatin1String("\"), (");

            if (ech == OtmParser::ExportVarEcho) {  // if |==$ then
                attr += QLatin1String("T_VARIANT(");
                attr += val;
                attr += QLatin1String(")");
            } else {
                attr += val;
            }
            attr += QLatin1String(")); ");
            attr += QLatin1String("echo(___pr.at(1).attributesString()); ");
            attr += (scriptArea ? RIGHT_DELIM_NO_TRIM : RIGHT_DELIM);
            e.attributes.clear();
            e.attributes << qMakePair(attr, QString());
            e.text = LEFT_DELIM;
            e.text += QLatin1String("eh(___pr.at(1).text); ");
            e.text += QLatin1String("echo(___pr.childElementsToString(1)); } while (0);");
            e.text += (scriptArea ? RIGHT_DELIM_NO_TRIM : RIGHT_DELIM);
        }

        // Sets attributes
        val = otmParser.getSrcCode(label, OtmParser::AttributeSet, &ech);  // + operator
        if (!val.isEmpty()) {
            QString s = generateErbPhrase(val, ech);
            e.setAttribute(s, QString());
        }

        // Replaces the element
        val = otmParser.getSrcCode(label, OtmParser::TagReplacement, &ech);  // : operator
        if (!val.isEmpty()) {
            // Sets the variable
            htmlParser.removeElementTree(i);
            e.text = generateErbPhrase(val, ech);
        } else {
            QStringList vals = otmParser.getWrapSrcCode(label, OtmParser::TagReplacement);
            if (!vals.isEmpty()) {
                // Adds block codes
                int idx = htmlParser.at(e.parent).children.indexOf(i);
                THtmlElement &he1 = htmlParser.insertNewElement(e.parent, idx);
                he1.text = LEFT_DELIM;
                he1.text += vals[0].trimmed();
                he1.text += (scriptArea ? RIGHT_DELIM_NO_TRIM : RIGHT_DELIM);

                QString s = vals.value(1).trimmed();
                if (!s.isEmpty()) {
                    THtmlElement &he2 = htmlParser.insertNewElement(e.parent, idx + 2);
                    he2.text = LEFT_DELIM;
                    he2.text += s;
                    he2.text += (scriptArea ? RIGHT_DELIM_NO_TRIM : RIGHT_DELIM);
                }
            }
        }
    }

    return htmlParser.toString();
}
