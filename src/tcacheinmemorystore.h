#ifndef TCACHEINMEMORYSTORE_H
#define TCACHEINMEMORYSTORE_H

#include "tcachesqlitestore.h"


class T_CORE_EXPORT TCacheInMemoryStore : public TCacheSQLiteStore
{
public:
    TCacheInMemoryStore(qint64 thresholdDbSize = 0);
    ~TCacheInMemoryStore() {}

    QString key() const { return QLatin1String("memory"); }
    QMap<QString, QVariant> defaultSettings() const override;
};

#endif // TCACHEINMEMORYSTORE_H
