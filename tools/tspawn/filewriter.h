/* Copyright (c) 2010-2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#ifndef FILEWRITER_H
#define FILEWRITER_H

#include <QString>


class FileWriter
{
public:
    FileWriter(const QString &filePath = QString());
    bool write(const QString &data, bool overwrite) const;
    QString fileName() const;
    void setFilePath(const QString &path) { filepath = path; }
    QString filePath() const { return filepath; }

private:
    bool write(const QString &data) const;
    QString filepath;
};

#endif // FILEWRITER_H
