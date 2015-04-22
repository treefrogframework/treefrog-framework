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

protected:
    void releaseKvsDatabases();
    void releaseSqlDatabases();

    QMap<int, QSqlDatabase> sqlDatabases;
    QMap<int, TKvsDatabase> kvsDatabases;

private:
    TSqlTransaction transactions;

    Q_DISABLE_COPY(TDatabaseContext)
};

#endif // TDATABASECONTEXT_H
