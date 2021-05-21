/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "viewconverter.h"
#include "erbconverter.h"
#include "otamaconverter.h"
#include <QDateTime>
#include <QFileInfo>
#include <QSet>
#include <QTextStream>

#define PROJECT_TEMPLATE                                                        \
    "TARGET = view\n"                                                           \
    "TEMPLATE = lib\n"                                                          \
    "CONFIG += shared debug\n"                                                  \
    "QT += network\n"                                                           \
    "QT -= gui\n"                                                               \
    "DEFINES += TF_DLL\n"                                                       \
    "INCLUDEPATH += ../../helpers ../../models\n"                               \
    "DEPENDPATH  += ../../helpers ../../models\n"                               \
    "DESTDIR = ../../../lib\n"                                                  \
    "LIBS += -L../../../lib -lhelper -lmodel\n"                                 \
    "QMAKE_CLEAN = *.cpp source.list\n\n"                                       \
    "tmake.target = source.list\n"                                              \
    "tmake.commands = tmake -f ../../../config/application.ini -v .. -d . -P\n" \
    "tmake.depends = qmake\n"                                                   \
    "QMAKE_EXTRA_TARGETS = tmake\n\n"                                           \
    "include(../../appbase.pri)\n"                                              \
    "!exists(source.list) {\n"                                                  \
    "  system( $$tmake.commands )\n"                                            \
    "}\n"                                                                       \
    "include(source.list)\n"

int defaultTrimMode;


ViewConverter::ViewConverter(const QDir &view, const QDir &output, bool projectFile) :
    viewDir(view), outputDir(output), createProFile(projectFile)
{
}


int ViewConverter::convertView(const QString &templateSystem) const
{
    const QStringList OtamaFilter(QLatin1String("*.") + OtamaConverter::fileSuffix());
    const QStringList ErbFilter(QLatin1String("*.") + ErbConverter::fileSuffix());

    QStringList classList;
    QStringList viewFiles;

    QDir helpersDir = viewDir;
    helpersDir.cdUp();
    helpersDir.cd("helpers");
    QDir partialDir = viewDir;
    partialDir.cd("partial");

    ErbConverter erbconv(outputDir, helpersDir, partialDir);
    OtamaConverter otamaconv(outputDir, helpersDir, partialDir);

    for (const QString &d : viewDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        // Reads erb-files
        QDir dir = viewDir;
        dir.cd(d);

        // Reads trim_mode file
        int trimMode = defaultTrimMode;
        QFile erbTrim(dir.absoluteFilePath(".trim_mode"));
        if (erbTrim.exists()) {
            if (erbTrim.open(QIODevice::ReadOnly)) {
                bool ok;
                int mode = erbTrim.readLine().trimmed().toInt(&ok);
                if (ok) {
                    trimMode = qMin(qMax(0, mode), 2);
                }
                erbTrim.close();
            }
        }

        QStringList filter;
        if (dir.dirName() == QLatin1String("mailer")) {
            filter = ErbFilter;
            trimMode = qMin(trimMode, 1);  // max of trim-mode of mailer is 1
        } else {
            filter = (templateSystem.toLower() == QLatin1String("otama")) ? OtamaFilter : ErbFilter;
        }

        // ERB or Otama
        for (const QFileInfo &fileinfo : dir.entryInfoList(filter, QDir::Files)) {
            bool convok = false;
            QString ext = fileinfo.suffix().toLower();
            if (ext == ErbConverter::fileSuffix()) {
                convok = erbconv.convert(fileinfo.absoluteFilePath(), trimMode);
                viewFiles << fileinfo.filePath();
            } else if (ext == OtamaConverter::fileSuffix()) {
                convok = otamaconv.convert(fileinfo.absoluteFilePath(), trimMode);
                viewFiles << fileinfo.filePath();
                QString otmFile = fileinfo.filePath().replace("." + OtamaConverter::fileSuffix(), "." + OtamaConverter::logicFileSuffix());
                viewFiles << otmFile;
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
    createSourceList(classList, viewFiles);
    return 0;
}


bool ViewConverter::createProjectFile() const
{
    return write(outputDir.filePath("_src.pro"), PROJECT_TEMPLATE);
}


bool ViewConverter::createSourceList(const QStringList &classNameList, const QStringList &viewFileList) const
{
    QString string;
    for (const auto &c : classNameList) {
        string += QLatin1String("HEADERS += ");
        string += c;
        string += QLatin1String(".moc\nSOURCES += ");
        string += c;
        string += QLatin1String(".cpp\n");
    }

    if (!viewFileList.isEmpty()) {
        string += QLatin1String("\n# include view files in the project\n");
        for (const auto &v : viewFileList) {
            string += QLatin1String("views.files += ");
            string += v;
            string += '\n';
        }
        string += QLatin1String("views.path = .dummy\n");
        string += QLatin1String("INSTALLS += views\n");
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
            qCritical("failed to open file: %s", qUtf8Printable(file.fileName()));
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
            std::printf("  created  %s\n", qUtf8Printable(outFile.fileName()));
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
    return fileInfo.absolutePath() + "/" + fileInfo.completeBaseName() + QLatin1Char('.') + ext;
}
