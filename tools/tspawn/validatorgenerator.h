#pragma once
#include <QtCore>


class ValidatorGenerator {
public:
    ValidatorGenerator(const QString &validator);
    bool generate(const QString &dst) const;

private:
    QString name;
};

