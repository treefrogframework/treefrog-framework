#pragma once
#include "servicegenerator.h"
#include <QDir>
#include <QPair>
#include <QString>
#include <QVariant>


class ViteVueServiceGenerator : public ServiceGenerator {
public:
    ViteVueServiceGenerator(const QString &service, const QList<QPair<QString, QMetaType::Type>> &fields, int pkIdx, int lockRevIdx);
    ~ViteVueServiceGenerator() { }

private:
    virtual QString headerFileTemplate() const;
    virtual QString sourceFileTemplate() const;
};
