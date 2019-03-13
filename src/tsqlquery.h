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
    QVariant value(const QString &name) const;

    static void clearCachedQueries();
    static QString escapeIdentifier(const QString &identifier, QSqlDriver::IdentifierType type = QSqlDriver::FieldName, int databaseId = 0);
    static QString escapeIdentifier(const QString &identifier, QSqlDriver::IdentifierType type, const QSqlDriver *driver);
    static QString formatValue(const QVariant &val, QVariant::Type type = QVariant::Invalid, int databaseId = 0);
    static QString formatValue(const QVariant &val, QVariant::Type type, const QSqlDatabase &database);
    static QString formatValue(const QVariant &val, const QSqlDatabase &database);
};


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
    return next() ? record().value(0) : QVariant();
}

/*!
  Returns the number of rows affected by the result's SQL statement, or -1
  if it cannot be determined. Note that for SELECT statements, the value is
  undefined; use size() instead. If the query is not active, -1 is returned.
 */
inline int TSqlQuery::numRowsAffected() const
{
    return QSqlQuery::numRowsAffected();
}

/*!
  Returns the size of the result (number of rows returned), or -1 if the size
  cannot be determined or if the database does not support reporting information
  about query sizes. Note that for non-SELECT statements (isSelect() returns
  false), size() will return -1. If the query is not active (isActive() returns
  false), -1 is returned.
  To determine the number of rows affected by a non-SELECT statement, use
  numRowsAffected().
 */
inline int TSqlQuery::size() const
{
    return QSqlQuery::size();
}

/*!
  Retrieves the next record in the result, if available, and positions the
  query on the retrieved record. Note that the result must be in the active
  state and isSelect() must return true before calling this function or it
  will do nothing and return false.
 */
inline bool TSqlQuery::next()
{
    return QSqlQuery::next();
}

/*!
  Returns the value of field index in the current record.
 */
inline QVariant TSqlQuery::value(int index) const
{
    return QSqlQuery::value(index);
}

/*!
  Returns the value of the field called name in the current record.
  If field name does not exist an invalid variant is returned.
 */
inline QVariant TSqlQuery::value(const QString &name) const
{
    return QSqlQuery::value(name);
}

#endif // TSQLQUERY_H
