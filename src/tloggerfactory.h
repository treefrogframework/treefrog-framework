#ifndef TLOGGERFACTORY_H
#define TLOGGERFACTORY_H

#include <QStringList>
#include <TGlobal>

class TLogger;


class T_CORE_EXPORT TLoggerFactory
{
public:
    static QStringList keys();
    static TLogger *create(const QString &key);

protected:
    static void loadPlugins();
};

#endif // TLOGGERFACTORY_H
