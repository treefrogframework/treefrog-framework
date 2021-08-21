#pragma once
#include <QMap>
#include <TSqlTransaction>
#include <TGlobal>

class QSqlDatabase;
class TKvsDatabase;


class T_CORE_EXPORT TDatabaseContext {
public:
    TDatabaseContext();
    virtual ~TDatabaseContext();

    QSqlDatabase &getSqlDatabase(int id = 0);
    TKvsDatabase &getKvsDatabase(Tf::KvsEngine engine);

    void setTransactionEnabled(bool enable, int id = 0);
    void release();
    void commitTransactions();
    bool commitTransaction(int id = 0);
    void rollbackTransactions();
    bool rollbackTransaction(int id = 0);
    int idleTime() const;
    static TDatabaseContext *currentDatabaseContext();
    static void setCurrentDatabaseContext(TDatabaseContext *context);

protected:
    void releaseKvsDatabases();
    void releaseSqlDatabases();

    QMap<int, TSqlTransaction> sqlDatabases;
    QMap<int, TKvsDatabase> kvsDatabases;

private:
    uint idleElapsed {0};

    T_DISABLE_COPY(TDatabaseContext)
    T_DISABLE_MOVE(TDatabaseContext)
};

