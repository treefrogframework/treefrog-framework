#ifndef VALIDATORGENERATOR_H
#define VALIDATORGENERATOR_H

#include <QtCore>


class ValidatorGenerator
{
public:
    ValidatorGenerator(const QString &validator, const QString &dst);
    bool generate() const;

private:
    QString name;
    QDir dstDir;
};

#endif // VALIDATORGENERATOR_H

