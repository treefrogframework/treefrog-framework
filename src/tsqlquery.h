#ifndef TSQLQUERY_H
#define TSQLQUERY_H

#include <QtSql>
#include <TGlobal>


class T_CORE_EXPORT TSqlQuery : public QSqlQuery
{
public:
    TSqlQuery(int databaseId = 0);
    TSqlQuery(QSqlDatabase db);

    TSqlQuery &prepare(const QString &query);
    bool load(const QString &filename);
    TSqlQuery &bind(const QString &placeholder, const QVariant &val);
    TSqlQuery &bind(int pos, const QVariant &val);
    TSqlQuery &addBind(const QVariant &val);
    QVariant getNextValue();
    QString queryDirPath() const;
    bool exec(const QString &query);
    bool exec();
    int numRowsAffected() const;
    int size() const;
    bool next();
    QVariant value(int index) const;

    static void clearCachedQueries();
    static QString escapeIdentifier(const QString &identifier, QSqlDriver::IdentifierType type = QSqlDriver::FieldName, int databaseId = 0);
    static QString escapeIdentifier(const QString &identifier, QSqlDriver::IdentifierType type, const QSqlDriver *driver);
    static QString formatValue(const QVariant &val, QVariant::Type type = QVariant::Invalid, int databaseId = 0);
    static QString formatValue(const QVariant &val, QVariant::Type type, const QSqlDatabase &database);
    static QString formatValue(const QVariant &val, const QSqlDatabase &database);
};


/*!
  Prepares the SQL query \a query for execution.
*/
inline TSqlQuery &TSqlQuery::prepare(const QString &query)
{
    QSqlQuery::prepare(query);
    return *this;
}

/*!
  Set the placeholder \a placeholder to be bound to value \a val in the
  prepared statement.
*/
inline TSqlQuery &TSqlQuery::bind(const QString &placeholder, const QVariant &val)
{
    QSqlQuery::bindValue(placeholder, val);
    return *this;
}

/*!
  Set the placeholder in position \a pos to be bound to value \a val in
  the prepared statement. Field numbering starts at 0.
*/
inline TSqlQuery &TSqlQuery::bind(int pos, const QVariant &val)
{
    QSqlQuery::bindValue(pos, val);
    return *this;
}

/*!
  Adds the value \a val to the list of values when using positional value
  binding and returns the query object. The order of the addBind() calls
  determines which placeholder a value will be bound to in the prepared
  query.
*/
inline TSqlQuery &TSqlQuery::addBind(const QVariant &val)
{
    QSqlQuery::addBindValue(val);
    return *this;
}

/*!
  Returns the value of first field in the next object and advances the
  internal iterator by one position. It can be used for a query returning
  at least one result, such as 'SELECT COUNT(*)'.
*/
inline QVariant TSqlQuery::getNextValue()
{
    return (next()) ? record().value(0) : QVariant();
}

#endif // TSQLQUERY_H
