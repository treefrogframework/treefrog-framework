#ifndef CONTROLLERGENERATOR_H
#define CONTROLLERGENERATOR_H

#include <QString>
#include <QDir>


class ControllerGenerator
{
public:
    ControllerGenerator(const QString &controller, const QString &table, const QStringList &actions, const QString &dst);
    ~ControllerGenerator() { }
    bool generate() const;

private:
    QString controllerName;
    QString tableName;
    QStringList actionList;
    QDir dstDir;
};

#endif // CONTROLLERGENERATOR_H
