#pragma once
#include "servicegenerator.h"
#include <QDir>
#include <QPair>
#include <QString>
#include <QVariant>


class VueServiceGenerator : public ServiceGenerator {
public:
    VueServiceGenerator(const QString &service, const QList<QPair<QString, QMetaType::Type>> &fields, int pkIdx, int lockRevIdx);
    ~VueServiceGenerator() { }

private:
    virtual QString headerFileTemplate() const;
    virtual QString sourceFileTemplate() const;
};
