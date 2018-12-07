#ifndef TSQLTRANSACTION_H
#define TSQLTRANSACTION_H

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
    TSqlTransaction(const TSqlTransaction &other);
    ~TSqlTransaction();

    QSqlDatabase &database() { return _database; }
    TSqlTransaction &operator=(const TSqlTransaction &);
    bool begin();
    bool commit();
    bool rollback();
    bool isActive() const { return _active; }
    void setEnabled(bool enable);
    void setDisabled(bool disable);

private:
    bool _enabled {true};
    QSqlDatabase _database;
    bool _active {false};
};


inline void TSqlTransaction::setEnabled(bool enable)
{
    _enabled = enable;
}


inline void TSqlTransaction::setDisabled(bool disable)
{
    _enabled = !disable;
}

#endif // TSQLTRANSACTION_H
