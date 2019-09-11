#include "tcacheinmemorystore.h"


TCacheInMemoryStore::TCacheInMemoryStore(qint64 thresholdDbSize) :
    TCacheSQLiteStore(thresholdDbSize)
{}


QMap<QString, QVariant> TCacheInMemoryStore::defaultSettings() const
{
    QMap<QString, QVariant> settings {
        {"DriverType", "QSQLITE"},
        {"DatabaseName", "file:memorydb?mode=memory&cache=shared"},
        {"ConnectOptions", "QSQLITE_OPEN_URI"},
        {"PostOpenStatements", "PRAGMA journal_mode=WAL; PRAGMA foreign_keys=ON; PRAGMA busy_timeout=5000; PRAGMA synchronous=NORMAL;"},
    };
    return settings;
}
