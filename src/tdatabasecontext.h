#ifndef TDATABASECONTEXT_H
#define TDATABASECONTEXT_H

#include <TGlobal>
#include <QSqlDatabase>
#include <TSqlTransaction>
#include <TKvsDatabase>


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

private:
    TSqlTransaction transactions;
    QMap<int, QSqlDatabase> sqlDatabases;
    QMap<int, TKvsDatabase> kvsDatabases;

    Q_DISABLE_COPY(TDatabaseContext)
};

#endif // TDATABASECONTEXT_H
