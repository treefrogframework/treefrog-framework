#pragma once
#include <QDir>
#include <QPair>
#include <QString>
#include <QVariant>


class ServiceGenerator {
public:
    ServiceGenerator(const QString &service, const QList<QPair<QString, QMetaType::Type>> &fields, int pkIdx, int lockRevIdx);
    ~ServiceGenerator() { }
    bool generate(const QString &dstDir) const;

private:
    virtual QString headerFileTemplate() const;
    virtual QString sourceFileTemplate() const;

    QString _serviceName;
    QString _tableName;
    QList<QPair<QString, QMetaType::Type>> _fieldList;
    int _pkIndex {0};
    int _lockRevIndex {0};
};

