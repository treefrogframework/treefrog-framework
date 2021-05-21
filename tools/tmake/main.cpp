/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "viewconverter.h"
#include <QCoreApplication>
#include <QFileInfo>
#include <QMap>
#include <QSettings>
#include <QString>
#include <QTextCodec>

constexpr auto DEFAULT_OUTPUT_DIR = "viewcodes";

extern QString devIni;
extern int defaultTrimMode;


static int usage()
{
    std::printf("usage: tmake [-f config-file] [-v view-dir] [-d output-dir] [-p|-P]\n");
    return 0;
}


static QMap<QString, QString> convertArgs(const QStringList &args)
{
    QMap<QString, QString> hash;
    for (int i = 1; i < args.count(); ++i) {
        if (args.value(i).startsWith('-')) {
            if (args.value(i + 1).startsWith('-')) {
                hash.insert(args.value(i), QString());
            } else {
                hash.insert(args.value(i), args.value(i + 1));
                ++i;
            }
        }
    }
    return hash;
}


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    int res = 0;
    QString appIni;
    QMap<QString, QString> args = convertArgs(QCoreApplication::arguments());

    if (!args.value("-f").isEmpty()) {
        appIni = args.value("-f");
        devIni = QFileInfo(appIni).dir().path() + "/development.ini";
    } else {
        const QString dir("../../config/");
        appIni = dir + "application.ini";
        devIni = dir + "development.ini";
    }

    if (!QFile::exists(appIni)) {
        usage();
        return 1;
    }

    QSettings appSetting(appIni, QSettings::IniFormat);
    QSettings devSetting(devIni, QSettings::IniFormat);

    // Default codec
    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    QString codecName = appSetting.value("InternalEncoding").toString();
    if (!codecName.isEmpty()) {
        QTextCodec *c = QTextCodec::codecForName(codecName.toLatin1().constData());
        if (c) {
            codec = c;
        }
    }
    QTextCodec::setCodecForLocale(codec);

    defaultTrimMode = devSetting.value("Erb.DefaultTrimMode", "1").toInt();
    std::printf("Erb.DefaultTrimMode: %d\n", defaultTrimMode);

    QDir viewDir(".");
    if (!args.value("-v").isEmpty()) {
        viewDir.setPath(args.value("-v"));
    }
    if (!viewDir.exists()) {
        usage();
        return 1;
    }

    QDir outputDir(DEFAULT_OUTPUT_DIR);
    if (!args.value("-d").isEmpty()) {
        outputDir.setPath(args.value("-d"));
    }

    if (outputDir.exists()) {
        if (outputDir.path() != ".") {
            std::printf("  exists   %s\n", qUtf8Printable(outputDir.path()));
        }
    } else {
        if (outputDir.mkpath(".")) {
            std::printf("  created  %s\n", qUtf8Printable(outputDir.path()));
        } else {
            usage();
            return 1;
        }
    }

    bool createProFile = (args.contains("-p") || !args.contains("-P"));
    ViewConverter conv(viewDir, outputDir, createProFile);
    QString templateSystem = devSetting.value("TemplateSystem").toString();
    if (templateSystem.isEmpty()) {
        templateSystem = appSetting.value("TemplateSystem", "Erb").toString();
    }

    res = conv.convertView(templateSystem);
    return res;
}
