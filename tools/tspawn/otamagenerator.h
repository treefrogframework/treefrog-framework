#ifndef OTAMAGENERATOR_H
#define OTAMAGENERATOR_H

#include <QStringList>
#include <QDir>


class OtamaGenerator
{
public:
    OtamaGenerator(const QString &view, const QString &table, const QString &dst);
    bool generate() const;

protected:
    QStringList generateViews() const;

private:
    QString viewName;
    QString tableName;
    QDir dstDir;
};

#endif // OTAMAGENERATOR_H
