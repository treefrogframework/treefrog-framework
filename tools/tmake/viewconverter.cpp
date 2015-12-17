/* Copyright (c) 2010-2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QTextStream>
#include <QDateTime>
#include <QFileInfo>
#include <QSet>
#include "viewconverter.h"
#include "erbconverter.h"
#include "otamaconverter.h"

#define PROJECT_TEMPLATE                                                \
    "TARGET = view\n"                                                   \
    "TEMPLATE = lib\n"                                                  \
    "CONFIG += shared debug\n"                                          \
    "QT += network\n"                                                   \
    "QT -= gui\n"                                                       \
    "DEFINES += TF_DLL\n"                                               \
    "INCLUDEPATH += ../../helpers ../../models\n"                       \
    "DEPENDPATH  += ../../helpers ../../models\n"                       \
    "DESTDIR = ../../../lib\n"                                          \
    "LIBS += -L../../../lib -lhelper -lmodel\n"                         \
    "QMAKE_CLEAN = *.cpp source.list\n\n"                               \
    "tmake.target = source.list\n"                                      \
    "tmake.commands = tmake -f ../../../config/application.ini -v .. -d . -P\n" \
    "tmake.depends = qmake\n"                                           \
    "QMAKE_EXTRA_TARGETS = tmake\n\n"                                   \
    "include(../../appbase.pri)\n"                                      \
    "!exists(source.list) {\n"                                          \
    "  system( $$tmake.commands )\n"                                    \
    "}\n"                                                               \
    "include(source.list)\n"


ViewConverter::ViewConverter(const QDir &view, const QDir &output, bool projectFile)
    : viewDir(view), outputDir(output), createProFile(projectFile)
{ }


int ViewConverter::convertView(const QString &templateSystem) const
{
    const QStringList OtamaFilter(QLatin1String("*.") + OtamaConverter::fileSuffix());
    const QStringList ErbFilter(QLatin1String("*.") + ErbConverter::fileSuffix());

    QStringList classList;

    QDir helpersDir = viewDir;
    helpersDir.cdUp();
    helpersDir.cd("helpers");

    ErbConverter erbconv(outputDir, helpersDir);
    OtamaConverter otamaconv(outputDir, helpersDir);

    foreach (QString d, viewDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        // Reads erb-files
        QDir dir = viewDir;
        dir.cd(d);

        // Reads trim_mode file
        int trimMode = -1;
        QFile erbTrim(dir.absoluteFilePath(".trim_mode"));
        if (erbTrim.exists()) {
            if (erbTrim.open(QIODevice::ReadOnly)) {
                bool ok;
                int mode = erbTrim.readLine().trimmed().toInt(&ok);
                if (ok) {
                    trimMode = mode;
                }
                erbTrim.close();
            }
        }

        QStringList filter;
        if (dir.dirName() == QLatin1String("mailer")) {
            filter = ErbFilter;
        } else {
            filter = (templateSystem.toLower() == QLatin1String("otama")) ? OtamaFilter : ErbFilter;
        }

        // ERB or Otama
        foreach (QFileInfo fileinfo, dir.entryInfoList(filter, QDir::Files)) {
            bool convok = false;
            QString ext = fileinfo.suffix().toLower();
            if (ext == ErbConverter::fileSuffix()) {
                convok = erbconv.convert(fileinfo.absoluteFilePath(), trimMode);
            } else if (ext == OtamaConverter::fileSuffix()) {
                convok = otamaconv.convert(fileinfo.absoluteFilePath());
            } else {
                continue;
            }

            QString className = getViewClassName(fileinfo);
            if (convok && !classList.contains(className)) {
                classList << className;
            }
        }
    }

    if (createProFile) {
        createProjectFile();
    }
    createSourceList(classList);
    return 0;
}


bool ViewConverter::createProjectFile() const
{
    return write(outputDir.filePath("_src.pro"), PROJECT_TEMPLATE);
}


bool ViewConverter::createSourceList(const QStringList &classNameList) const
{
    QString string;
    for (QStringListIterator i(classNameList); i.hasNext(); ) {
        const QString &c = i.next();
        string += QLatin1String("HEADERS += ");
        string += c;
        string += QLatin1String(".moc\nSOURCES += ");
        string += c;
        string += QLatin1String(".cpp\n");
    }
    return write(outputDir.filePath("source.list"), string);
}


bool ViewConverter::write(const QString &filePath, const QString &data) const
{
    QString orig;
    QFile outFile(filePath);

    if (outFile.exists()) {
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            orig = file.readAll();
            file.close();
        } else {
            qCritical("failed to open file: %s", qPrintable(file.fileName()));
            return false;
        }
    }

    if (data != orig) {
        if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qCritical("failed to create file");
            return false;
        }
        QTextStream ts(&outFile);
        ts << data;
        if (ts.status() == QTextStream::Ok) {
            printf("  created  %s\n", qPrintable(outFile.fileName()));
        } else {
            return false;
        }
    }
    return true;
}


QString ViewConverter::getViewClassName(const QString &filePath)
{
    return getViewClassName(QFileInfo(filePath));
}


QString ViewConverter::getViewClassName(const QFileInfo &fileInfo)
{
    return fileInfo.dir().dirName() + QLatin1Char('_') + fileInfo.completeBaseName() + QLatin1String("View");
}


QString ViewConverter::changeFileExtension(const QString &filePath, const QString &ext)
{
    QFileInfo fileInfo(filePath);
    return fileInfo.absolutePath() + QDir::separator() + fileInfo.completeBaseName() + QLatin1Char('.') + ext;
}
