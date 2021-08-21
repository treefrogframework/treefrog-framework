/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "filewriter.h"
#include <QtCore>
#ifndef Q_CC_MSVC
#include <unistd.h>
#endif

namespace {
bool gOverwrite = false;

QString diff(const QString &data1, const QString &data2)
{
    QTemporaryFile f1, f2;
    if (!f1.open() || f1.write(data1.toUtf8()) < 0) {
        return QString();
    }

    if (!f2.open() || f2.write(data2.toUtf8()) < 0) {
        return QString();
    }

    f1.close();
    f2.close();

    if (!QFileInfo(f1).exists() || !QFileInfo(f2).exists()) {
        qCritical("Intarnal error [%s:%d]", __FILE__, __LINE__);
        return QString();
    }

    QProcess df;
    QStringList args;
    args << "-u" << f1.fileName() << f2.fileName();
    df.start("diff", args);

    if (!df.waitForStarted() || !df.waitForFinished()) {
        return QString();
    }
    return QString::fromUtf8(df.readAll().data());
}
}


FileWriter::FileWriter(const QString &filePath) :
    filepath(filePath)
{
}


static QString readFile(const QString &fileName)
{
    QString ret;
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical("failed to create file: %s", qUtf8Printable(fileName));
        return ret;
    }

    ret = file.readAll();  // Use the codec of QTextCodec::codecForCStrings()
    file.close();
    return ret;
}


bool FileWriter::write(const QString &data, bool overwrite) const
{
    if (filepath.isEmpty()) {
        return false;
    }

    bool res = false;
    QByteArray act;
    QFileInfo fi(filepath);

    if (!fi.absoluteDir().exists()) {
        fi.absoluteDir().mkpath(".");
    }

    if (fi.exists()) {
        QString orig = readFile(filepath);
        if (orig == data) {
            std::printf("  unchanged %s\n", qUtf8Printable(QDir::cleanPath(fi.filePath())));
            return true;
        }

        if (gOverwrite || overwrite) {
            QFile::remove(filepath);
            res = write(data);
            act = (res) ? "updated " : "error   ";

        } else {
            QTextStream stream(stdin);
            for (;;) {
                std::printf("  overwrite %s? [ynaqdh] ", qUtf8Printable(QDir::cleanPath(fi.filePath())));

                QString line = stream.readLine();
                if (line.isNull()) {
                    break;
                }

                if (line.isEmpty()) {
                    continue;
                }

                const QChar c = line[0];
                if (c == 'Y' || c == 'y') {
                    QFile::remove(filepath);
                    res = write(data);
                    act = (res) ? "updated " : "error   ";
                    break;

                } else if (c == 'N' || c == 'n') {
                    return true;

                } else if (c == 'A' || c == 'a') {
                    gOverwrite = true;
                    QFile::remove(filepath);
                    res = write(data);
                    act = (res) ? "updated " : "error   ";
                    break;

                } else if (c == 'Q' || c == 'q') {
                    ::_exit(1);
                    return false;

                } else if (c == 'D' || c == 'd') {
                    std::printf("-----------------------------------------------------------\n");
                    orig = readFile(filepath);  // Re-read
                    QString df = diff(orig, data);
                    if (df.isEmpty()) {
                        qCritical("Error: diff command failed");
                    } else {
                        std::printf("%s", qUtf8Printable(df));
                        std::printf("-----------------------------------------------------------\n");
                    }

                } else if (c == 'H' || c == 'h') {
                    std::printf("   y - yes, overwrite\n");
                    std::printf("   n - no, do not overwrite\n");
                    std::printf("   a - all, overwrite this and all others\n");
                    std::printf("   q - quit, abort\n");
                    std::printf("   d - diff, show the differences between the old and the new\n");
                    std::printf("   h - help, show this help\n\n");

                } else {
                    // one more
                }
            }
        }
    } else {
        res = write(data);
        act = (res) ? "created " : "error   ";
    }

    std::printf("  %s  %s\n", act.data(), qUtf8Printable(QDir::cleanPath(fi.filePath())));
    return res;
}


bool FileWriter::write(const QString &data) const
{
    if (filepath.isEmpty()) {
        return false;
    }

    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCritical("failed to create file: %s", qUtf8Printable(QDir::cleanPath(filepath)));
        return false;
    }

    QTextStream ts(&file);
    ts << data;
    file.close();
    return (ts.status() == QTextStream::Ok);
}


QString FileWriter::fileName() const
{
    return QFileInfo(filepath).fileName();
}
