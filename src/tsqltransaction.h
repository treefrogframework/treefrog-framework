#pragma once
#include <QSqlDatabase>
#include <TGlobal>

/*!
  \class Transaction
  \brief The Transaction class provides a transaction of database.
*/

class T_CORE_EXPORT TSqlTransaction {
public:
    TSqlTransaction();
    TSqlTransaction(const TSqlTransaction &other) = default;
    ~TSqlTransaction();

    TSqlTransaction &operator=(const TSqlTransaction &) = default;
    QSqlDatabase &database() { return _database; }
    bool begin();
    bool commit();
    bool rollback();
    bool isActive() const { return _active; }
    void setEnabled(bool enable);
    void setDisabled(bool disable);

private:
    QSqlDatabase _database;
    bool _enabled {true};
    bool _active {false};
    QString _connectionName;
};


inline void TSqlTransaction::setEnabled(bool enable)
{
    _enabled = enable;
}


inline void TSqlTransaction::setDisabled(bool disable)
{
    _enabled = !disable;
}

