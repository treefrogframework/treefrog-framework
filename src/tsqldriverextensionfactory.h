#ifndef TSQLDRIVEREXTENSIONFACTORY_H
#define TSQLDRIVEREXTENSIONFACTORY_H

#include <QStringList>
#include <TGlobal>

class TSqlDriverExtension;


class T_CORE_EXPORT TSqlDriverExtensionFactory
{
public:
    static QStringList keys() { return QStringList(); }
    static TSqlDriverExtension *create(const QString &key) { return 0; }
    static void destroy(const QString &key, TSqlDriverExtension *extension) { }
};

#endif // TSQLDRIVEREXTENSIONFACTORY_H
