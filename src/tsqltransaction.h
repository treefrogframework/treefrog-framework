#ifndef TSQLTRANSACTION_H
#define TSQLTRANSACTION_H

#include <QVector>
#include <QSqlDatabase>
#include <TGlobal>

/*!
  \class Transaction
  \brief The Transaction class provides a transaction of database.
*/

class T_CORE_EXPORT TSqlTransaction
{
public:
    TSqlTransaction();
    ~TSqlTransaction();
    bool begin(QSqlDatabase &database);
    void commitAll();
    bool commit(int id);
    void rollbackAll();
    bool rollback(int id);
    void setEnabled(bool enable);
    void setDisabled(bool disable);

private:
    bool enabled;
    QVector<QSqlDatabase> databases;
};


inline void TSqlTransaction::setEnabled(bool enable)
{
    enabled = enable;
}


inline void TSqlTransaction::setDisabled(bool disable)
{
    enabled = !disable;
}

#endif // TSQLTRANSACTION_H
