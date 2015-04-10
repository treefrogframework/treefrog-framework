#ifndef PROJECTFILEGENERATOR_H
#define PROJECTFILEGENERATOR_H

#include <QStringList>


class ProjectFileGenerator
{
public:
    ProjectFileGenerator(const QString &path) : filePath(path) { }
    QString path() const { return filePath; }
    bool exists() const;
    bool add(const QStringList &files) const;
    bool remove(const QStringList &files) const;

private:
    QString filePath;
};

#endif // PROJECTFILEGENERATOR_H
