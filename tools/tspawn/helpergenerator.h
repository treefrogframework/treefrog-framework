#pragma once
#include <QtCore>


class HelperGenerator {
public:
    HelperGenerator(const QString &name);
    bool generate(const QString &dst) const;

private:
    QString name;
};

