/* Copyright (c) 2010, AOYAMA Kazuharu
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
    FileWriter(const QString &fileName = QString());
    bool write(const QString &data, bool overwrite) const;
    QString fileName() const { return filename; }
    void setFileName(const QString &name) { filename = name; }

private:
    bool write(const QString &data) const;
    QString filename;
};

#endif // FILEWRITER_H
