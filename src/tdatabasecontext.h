#ifndef TDATABASECONTEXT_H
#define TDATABASECONTEXT_H

#include <QMap>
#include <QSqlDatabase>
#include <TSqlTransaction>
#include <TKvsDatabase>
#include <TGlobal>


class T_CORE_EXPORT TDatabaseContext
{
public:
    TDatabaseContext();
    virtual ~TDatabaseContext();

    QSqlDatabase &getSqlDatabase(int id);
    TKvsDatabase &getKvsDatabase(TKvsDatabase::Type type);

    void setTransactionEnabled(bool enable);
    void release();
    bool beginTransaction(QSqlDatabase &database);
    void commitTransactions();
    void rollbackTransactions();
    int idleTime() const;

protected:
    void releaseKvsDatabases();
    void releaseSqlDatabases();

    QMap<int, QSqlDatabase> sqlDatabases;
    QMap<int, TKvsDatabase> kvsDatabases;

private:
    TSqlTransaction transactions;
    uint idleElapsed {0};

    T_DISABLE_COPY(TDatabaseContext)
    T_DISABLE_MOVE(TDatabaseContext)
};

#endif // TDATABASECONTEXT_H
