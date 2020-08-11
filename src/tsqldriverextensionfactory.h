#pragma once
#include <QStringList>
#include <TGlobal>

class TSqlDriverExtension;
class QSqlDriver;


class T_CORE_EXPORT TSqlDriverExtensionFactory {
public:
    static QStringList keys();
    static TSqlDriverExtension *create(const QString &key, const QSqlDriver *driver);
    static void destroy(const QString &key, TSqlDriverExtension *extension);
};

