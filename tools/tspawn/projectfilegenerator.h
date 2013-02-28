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
