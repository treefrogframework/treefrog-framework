#pragma once
#include "erbgenerator.h"
#include <QDir>
#include <QPair>
#include <QStringList>
#include <QVariant>


class VueErbGenerator : public ErbGenerator {
public:
    VueErbGenerator(const QString &view, const QList<QPair<QString, QMetaType::Type>> &fields, int pkIdx, int autoValIdx);
    virtual ~VueErbGenerator() {}

private:
    virtual QString indexTemplate() const;
    virtual QString showTemplate() const;
    virtual QString createTemplate() const;
    virtual QString saveTemplate() const;
    virtual PlaceholderList replaceList() const;
};
