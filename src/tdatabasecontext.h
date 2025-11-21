#pragma once
#include <TSqlDatabase>
#include <TSqlTransaction>
#include <TKvsDatabase>
#include <TGlobal>
#include "tsqldatabasepool.h"
#include <vector>

class QSqlDatabase;
class TCache;


class T_CORE_EXPORT TDatabaseContext {
public:
    TDatabaseContext();
    virtual ~TDatabaseContext();
    TDatabaseContext(TDatabaseContext &&) = default;
    TDatabaseContext &operator=(TDatabaseContext &&) = default;

    TSqlDatabase &getSqlDatabase(int id = 0);
    TKvsDatabase &getKvsDatabase(Tf::KvsEngine engine);

    void setTransactionEnabled(bool enable, int id = 0);
    void release();
    void commitTransactions();
    bool commitTransaction(int id = 0);
    void rollbackTransactions();
    bool rollbackTransaction(int id = 0);
    int idleTime() const;
    TCache *cache();
    static TDatabaseContext *currentDatabaseContext();
    static void setCurrentDatabaseContext(TDatabaseContext *context);

protected:
    void releaseKvsDatabases();
    void releaseSqlDatabases();

    std::vector<TSqlTransaction> sqlTransactions;
    std::vector<TKvsDatabase::Handle> kvsDatabases;

private:
    uint idleElapsed {0};
    TCache *cachep {nullptr};

    T_DISABLE_COPY(TDatabaseContext)
};

