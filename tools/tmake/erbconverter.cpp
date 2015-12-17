/* Copyright (c) 2010-2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QFileInfo>
#include <QDateTime>
#include <QTextStream>
#include "erbconverter.h"
#include "erbparser.h"
#include "viewconverter.h"

#define VIEW_SOURCE_TEMPLATE                                    \
    "#include <QtCore>\n"                                       \
    "#include <TreeFrogView>\n"                                 \
    "%4"                                                        \
    "\n"                                                        \
    "class T_VIEW_EXPORT %1 : public TActionView\n"             \
    "{\n"                                                       \
    "  Q_OBJECT\n"                                              \
    "public:\n"                                                 \
    "  %1() : TActionView() { }\n"                              \
    "  %1(const %1 &) : TActionView() { }\n"                    \
    "  QString toString();\n"                                   \
    "};\n"                                                      \
    "\n"                                                        \
    "QString %1::toString()\n"                                  \
    "{\n"                                                       \
    "  responsebody.reserve(%3);\n"                             \
    "%2\n"                                                      \
    "  return responsebody;\n"                                  \
    "}\n"                                                       \
    "\n"                                                        \
    "Q_DECLARE_METATYPE(%1)\n"                                  \
    "T_REGISTER_VIEW(%1)\n"                                     \
    "\n"                                                        \
    "#include \"%1.moc\"\n"

int defaultTrimMode;


ErbConverter::ErbConverter(const QDir &output, const QDir &helpers)
    : outputDirectory(output), helpersDirectory(helpers)
{ }


bool ErbConverter::convert(const QString &erbPath, int trimMode) const
{
    // Sets trim mode
    if (trimMode < 0)
        trimMode = defaultTrimMode;

    QFile erbFile(erbPath);
    QString className = ViewConverter::getViewClassName(erbPath);
    QFile outFile(outputDirectory.filePath(className + ".cpp"));
    
    // Checks file's timestamp
    QFileInfo erbFileInfo(erbFile);
    QFileInfo outFileInfo(outFile);
    if (erbFileInfo.exists() && outFileInfo.exists()) {
        if (outFileInfo.lastModified() > erbFileInfo.lastModified()) {
            //printf("  done    %s\n", qPrintable(outFile.fileName()));
            return true;
        } else {
            if (outFile.remove()) {
                printf("  removed  %s\n", qPrintable(outFile.fileName()));
            }
        }
    }
    
    if (!erbFile.open(QIODevice::ReadOnly)) {
        qCritical("failed to read html.erb file : %s", qPrintable(erbFile.fileName()));
        return false;
    }

    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCritical("failed to create file");
        return false;
    }

    ErbParser parser((ErbParser::TrimMode)trimMode);
    parser.parse(QTextStream(&erbFile).readAll());
    QString code = parser.sourceCode();
    QTextStream ts(&outFile);
    ts << QString(VIEW_SOURCE_TEMPLATE).arg(className, code, QString::number(code.size()), generateIncludeCode(parser));
    if (ts.status() == QTextStream::Ok) {
        printf("  created  %s\n", qPrintable(outFile.fileName()));
    }
    return true;
}


bool ErbConverter::convert(const QString &className, const QString &erb) const
{
    QFile outFile(outputDirectory.filePath(className + ".cpp"));
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCritical("failed to create file");
        return false;
    }

    ErbParser parser((ErbParser::TrimMode)defaultTrimMode);
    parser.parse(erb);
    QString code = parser.sourceCode();
    QTextStream ts(&outFile);
    ts << QString(VIEW_SOURCE_TEMPLATE).arg(className, code, QString::number(code.size()), generateIncludeCode(parser));
    if (ts.status() == QTextStream::Ok) {
        printf("  created  %s\n", qPrintable(outFile.fileName()));
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
    filter << "*.h" << "*.hh" << "*.hpp" << "*.hxx";
    foreach (QString f, helpersDirectory.entryList(filter, QDir::Files)) {
        code += "#include \"";
        code += f;
        code += "\"\n";
    }
    return code;
}
