#include "tcacheinmemorystore.h"


TCacheInMemoryStore::TCacheInMemoryStore() :
    TCacheSQLiteStore("file:memorydb?mode=memory&cache=shared", "QSQLITE_OPEN_URI", 0)
{}
