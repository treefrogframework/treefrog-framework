/* Copyright (c) 2011, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#ifndef PROJECTFILEGENERATOR_H
#define PROJECTFILEGENERATOR_H

#include <QStringList>


class ProjectFileGenerator
{
public:
    ProjectFileGenerator(const QString &path) : filePath(path) { }
    QString path() const { return filePath; }
    bool exists() const;
    bool add(const QStringList &files);
    bool remove(const QStringList &files);
    
private:
    QString filePath;
};

#endif // PROJECTFILEGENERATOR_H
