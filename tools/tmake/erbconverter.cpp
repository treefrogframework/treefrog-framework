/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "erbconverter.h"
#include "erbparser.h"
#include "viewconverter.h"
#include <QDateTime>
#include <QFileInfo>
#include <QTextStream>
#include <QRegularExpression>

#define VIEW_SOURCE_TEMPLATE                        \
    "#include <QtCore>\n"                           \
    "#include <TreeFrogView>\n"                     \
    "%4"                                            \
    "\n"                                            \
    "class T_VIEW_EXPORT %1 : public TActionView\n" \
    "{\n"                                           \
    "  Q_OBJECT\n"                                  \
    "public:\n"                                     \
    "  %1() : TActionView() { }\n"                  \
    "  QString toString();\n"                       \
    "};\n"                                          \
    "\n"                                            \
    "QString %1::toString()\n"                      \
    "{\n"                                           \
    "  responsebody.reserve(%3);\n"                 \
    "%2\n"                                          \
    "  return responsebody;\n"                      \
    "}\n"                                           \
    "\n"                                            \
    "T_DEFINE_VIEW(%1)\n"                           \
    "\n"                                            \
    "#include \"%1.moc\"\n"


const QRegularExpression RxPartialTag("<%#partial[ \t]+\"([^\"]+)\"[ \t]*%>");


ErbConverter::ErbConverter(const QDir &output, const QDir &helpers, const QDir &partial) :
    outputDirectory(output), helpersDirectory(helpers), partialDirectory(partial)
{
}


bool ErbConverter::convert(const QString &erbPath, int trimMode) const
{
    QFile erbFile(erbPath);
    QString className = ViewConverter::getViewClassName(erbPath);
    QFile outFile(outputDirectory.filePath(className + ".cpp"));

    // Checks file's timestamp
    QFileInfo erbFileInfo(erbFile);
    QFileInfo outFileInfo(outFile);

    if (!erbFile.open(QIODevice::ReadOnly)) {
        qCritical("failed to read template.erb file : %s", qUtf8Printable(erbFile.fileName()));
        return false;
    }

    QString erbSrc = QTextStream(&erbFile).readAll();
    auto partialList = replacePartialTag(erbSrc, 0);

    QDateTime latestPartialTs;
    for (const auto &file : partialList) {
        auto ts = QFileInfo(partialDirectory.filePath(file)).lastModified();
        if (ts.isValid() && ts > latestPartialTs) {
            latestPartialTs = ts;
        }
    }

    // Checks timestamps
    if (outFileInfo.exists()) {
        if ((latestPartialTs.isValid() && latestPartialTs >= outFileInfo.lastModified())
            || erbFileInfo.lastModified() >= outFileInfo.lastModified()) {
            if (outFile.remove()) {
                std::printf("  removed  %s\n", qUtf8Printable(outFile.fileName()));
            }
        } else {
            //std::printf("  done    %s\n", qUtf8Printable(outFile.fileName()));
            return true;
        }
    }

    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCritical("failed to create file");
        return false;
    }

    ErbParser parser((ErbParser::TrimMode)trimMode);
    parser.parse(erbSrc);
    QString code = parser.sourceCode();
    QTextStream ts(&outFile);
    ts << QString(VIEW_SOURCE_TEMPLATE).arg(className, code, QString::number(code.size()), generateIncludeCode(parser));
    if (ts.status() == QTextStream::Ok) {
        std::printf("  created  %s  (trim:%d)\n", qUtf8Printable(outFile.fileName()), trimMode);
    }
    return true;
}


bool ErbConverter::convert(const QString &className, const QString &erb, int trimMode) const
{
    QFile outFile(outputDirectory.filePath(className + ".cpp"));
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCritical("failed to create file");
        return false;
    }

    ErbParser parser((ErbParser::TrimMode)trimMode);
    parser.parse(erb);
    QString code = parser.sourceCode();
    QTextStream ts(&outFile);
    ts << QString(VIEW_SOURCE_TEMPLATE).arg(className, code, QString::number(code.size()), generateIncludeCode(parser));
    if (ts.status() == QTextStream::Ok) {
        std::printf("  created  %s  (trim:%d)\n", qUtf8Printable(outFile.fileName()), trimMode);
    }
    return true;
}


QString ErbConverter::escapeNewline(const QString &string)
{
    QString str;
    str.reserve(string.length() * 1.1);

    for (auto &s : string) {
        if (s == QLatin1Char('\\')) {
            str += QLatin1String("\\\\");
        } else if (s == QLatin1Char('\n')) {
            str += QLatin1String("\\n");
        } else if (s == QLatin1Char('\r')) {
            str += QLatin1String("\\r");
        } else if (s == QLatin1Char('"')) {
            str += QLatin1String("\\\"");
        } else {
            str += s;
        }
    }
    return str;
}


QString ErbConverter::generateIncludeCode(const ErbParser &parser) const
{
    QString code = parser.includeCode();
    QStringList filter;
    filter << "*.h"
           << "*.hh"
           << "*.hpp"
           << "*.hxx";
    foreach (QString f, helpersDirectory.entryList(filter, QDir::Files)) {
        code += "#include \"";
        code += f;
        code += "\"\n";
    }
    return code;
}


QStringList ErbConverter::replacePartialTag(QString &erbSrc, int depth) const
{
    QStringList ret;  // partial files replaced
    QString erbReplaced;
    int pos = 0;

    while (pos < erbSrc.length()) {
        auto match = RxPartialTag.match(erbSrc, pos);
        if (!match.hasMatch()) {
            erbReplaced += erbSrc.mid(pos);
            break;
        }

        int idx = match.capturedStart();
        QString partialFile = match.captured(1);
        if (QFileInfo(partialFile).suffix().toLower() != "erb") {
            partialFile += ".erb";
        }

        if (depth > 10) {
            // no more replace
            qWarning("Partial template '%s' infinitely recursively included?", partialFile.toLocal8Bit().data());
            return ret;
        }

        erbReplaced += erbSrc.mid(pos, idx - pos);
        pos = idx + match.capturedLength();

        // Includes the partial
        QFile partErb(partialDirectory.filePath(partialFile));
        if (partErb.open(QIODevice::ReadOnly)) {
            ret << partialFile;
            QString part = QTextStream(&partErb).readAll();
            ret << replacePartialTag(part, depth + 1);
            erbReplaced += part;
        }
    }

    erbSrc = erbReplaced;
    ret.removeDuplicates();
    return ret;
}
