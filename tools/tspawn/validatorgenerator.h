#ifndef VALIDATORGENERATOR_H
#define VALIDATORGENERATOR_H

#include <QtCore>


class ValidatorGenerator
{
public:
    ValidatorGenerator(const QString &validator);
    bool generate(const QString &dst) const;

private:
    QString name;
};

#endif // VALIDATORGENERATOR_H

