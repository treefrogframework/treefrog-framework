#include "tcacheinmemorystore.h"


TCacheInMemoryStore::TCacheInMemoryStore(qint64 thresholdDbSize) :
    TCacheSQLiteStore("file:memorydb?mode=memory&cache=shared", "QSQLITE_OPEN_URI", thresholdDbSize)
{}
