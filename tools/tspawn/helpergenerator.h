#ifndef HELPERGENERATOR_H
#define HELPERGENERATOR_H

#include <QtCore>


class HelperGenerator
{
public:
    HelperGenerator(const QString &name);
    bool generate(const QString &dst) const;

private:
    QString name;
};

#endif // HELPERGENERATOR_H
