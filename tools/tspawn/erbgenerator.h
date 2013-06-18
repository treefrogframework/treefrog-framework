#ifndef ERBGENERATOR_H
#define ERBGENERATOR_H

#include <QStringList>
#include <QDir>


class ErbGenerator
{
public:
    ErbGenerator(const QString &view, const QString &table, const QString &dst);
    bool generate() const;

private:
    QString viewName;
    QString tableName;
    QDir dstDir;
};

#endif // ERBGENERATOR_H
