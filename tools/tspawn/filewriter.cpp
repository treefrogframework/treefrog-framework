/* Copyright (c) 2010-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QtCore>
#include "filewriter.h"
#ifndef Q_CC_MSVC
# include <unistd.h>
#endif

static bool gOverwrite = false;


static QString diff(const QString &data1, const QString &data2)
{
    QTemporaryFile f1, f2;
    if (!f1.open() || f1.write(data1.toUtf8()) < 0)
        return QString();

    if (!f2.open() || f2.write(data2.toUtf8()) < 0)
        return QString();

    f1.close();
    f2.close();

    if (!f1.exists() || !f2.exists()) {
        qCritical("Intarnal error [%s:%d]", __FILE__, __LINE__);
        return QString();
    }
    
    QProcess df;
    QStringList args;
    args << "-u" << f1.fileName() << f2.fileName();
    df.start("diff", args);
    
    if (!df.waitForStarted())
        return QString();

    if (!df.waitForFinished())
        return QString();
    
    return QString::fromUtf8(df.readAll().data());
}


FileWriter::FileWriter(const QString &filePath)
    : filepath(filePath)
{ }


static QString readFile(const QString &fileName)
{
    QString ret;
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical("failed to create file: %s", qPrintable(fileName));
        return ret;
    }
    
    ret = file.readAll(); // Use the codec of QTextCodec::codecForCStrings()
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

    if (fi.exists()) {
        QString orig = readFile(filepath);
        if (orig == data) {
            printf("  unchanged %s\n", qPrintable(QDir::cleanPath(fi.filePath())));
            return true;
        }

        if (gOverwrite || overwrite) {
            QFile::remove(filepath);
            res = write(data);
            act = (res) ? "updated " : "error   ";

        } else {
            QTextStream stream(stdin);
            for (;;) {
                printf("  overwrite %s? [ynaqdh] ", qPrintable(QDir::cleanPath(fi.filePath())));
                
                QString line = stream.readLine();
                if (line.isNull())
                    break;

                if (line.isEmpty())
                    continue;

                QCharRef c = line[0];
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
                    printf("-----------------------------------------------------------\n");
                    orig = readFile(filepath);  // Re-read
                    QString df = diff(orig, data);
                    if (df.isEmpty()) {
                        qCritical("Error: diff command not found");
                    } else {
                        printf("%s", qPrintable(df));
                        printf("-----------------------------------------------------------\n");
                    }

                } else if (c == 'H' || c == 'h') {
                    printf("   y - yes, overwrite\n");
                    printf("   n - no, do not overwrite\n");
                    printf("   a - all, overwrite this and all others\n");
                    printf("   q - quit, abort\n");
                    printf("   d - diff, show the differences between the old and the new\n");
                    printf("   h - help, show this help\n\n");

                } else {
                    // one more
                }
            }
        }
    } else {
        res = write(data);
        act = (res) ? "created " : "error   ";
    }

    printf("  %s  %s\n", act.data(), qPrintable(QDir::cleanPath(fi.filePath())));
    return res;
}


bool FileWriter::write(const QString &data) const
{
    if (filepath.isEmpty()) {
        return false;
    }

    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCritical("failed to create file: %s", qPrintable(QDir::cleanPath(filepath)));
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
