#ifndef TMAILERFACTORY_H
#define TMAILERFACTORY_H

#include <QStringList>
#include <TGlobal>

class TMailer;


class T_CORE_EXPORT TMailerFactory
{
public:
    static QStringList keys();
    static TMailer *create(const QString &key);

protected:
    enum DriverType {
        Invalid = 0,
        Smtp,
        Plugin,
    };

    static void loadPlugins();
};

#endif // TMAILERFACTORY_H
