#ifndef TSQLORMAPPER_H
#define TSQLORMAPPER_H

#include <QtSql>
#include <QList>
#include <QMap>
#include <TGlobal>
#include <TSqlObject>
#include <TCriteria>
#include <TCriteriaConverter>
#include <TSqlQuery>
#include "tsystemglobal.h"

/*!
  \class TSqlORMapper
  \brief The TSqlORMapper class is a template class that provides
  concise functionality to object-relational mapping.
  It can be used to retrieve TSqlObject objects with a TCriteria
  from a table.
  \sa TSqlObject, TCriteria
*/


template <class T>
class TSqlORMapper : public QSqlTableModel
{
public:
    TSqlORMapper();
    virtual ~TSqlORMapper();

    void setLimit(int limit);
    void setOffset(int offset);
    void setSortOrder(int column, TSql::SortOrder order);  // obsoleted
    void setSortOrder(int column, Tf::SortOrder order);
    void reset();

    T findFirst(const TCriteria &cri = TCriteria());
    T findFirstBy(int column, QVariant value);
    T findByPrimaryKey(QVariant pk);
    int find(const TCriteria &cri = TCriteria());
    int findBy(int column, QVariant value);
    int findIn(int column, const QVariantList &values);
    T first() const;
    T last() const;
    T value(int i) const;

    int findCount(const TCriteria &cri = TCriteria());
    int findCountBy(int column, QVariant value);
    QList<T> findAll(const TCriteria &cri = TCriteria());
    QList<T> findAllBy(int column, QVariant value);
    QList<T> findAllIn(int column, const QVariantList &values);
    int updateAll(const TCriteria &cri, int column, QVariant value);
    int updateAll(const TCriteria &cri, const QMap<int, QVariant> &values);
    int removeAll(const TCriteria &cri = TCriteria());

protected:
    void setFilter(const QString &filter);
    QString orderBy() const;
    virtual QString orderByClause() const { return QString(); }
    virtual void clear();
    virtual QString selectStatement() const;

private:
    Q_DISABLE_COPY(TSqlORMapper)

    QString queryFilter;
    int sortColumn;
    Tf::SortOrder sortOrder;
    int queryLimit;
    int queryOffset;
};


/*!
  Constructor.
*/
template <class T>
inline TSqlORMapper<T>::TSqlORMapper()
    : QSqlTableModel(0, Tf::currentSqlDatabase(T().databaseId())),
      sortColumn(-1), sortOrder(Tf::AscendingOrder), queryLimit(0),
      queryOffset(0)
{
    setTable(T().tableName());
}

/*!
  Destructor.
*/
template <class T>
inline TSqlORMapper<T>::~TSqlORMapper()
{ }

/*!
  Returns the first ORM object retrieved with the criteria \a cri from
  the table.
*/
template <class T>
inline T TSqlORMapper<T>::findFirst(const TCriteria &cri)
{
    if (!cri.isEmpty()) {
        TCriteriaConverter<T> conv(cri, database());
        setFilter(conv.toString());
    } else {
        setFilter(QString());
    }

    int oldLimit = queryLimit;
    queryLimit = 1;
    bool ret = select();
    tWriteQueryLog(query().lastQuery(), ret, lastError());
    queryLimit = oldLimit;

    tSystemDebug("rowCount: %d", rowCount());
    return first();
}

/*!
  Returns the first ORM object retrieved with the criteria for the
  \a column as the \a value in the table.
*/
template <class T>
inline T TSqlORMapper<T>::findFirstBy(int column, QVariant value)
{
    return findFirst(TCriteria(column, value));
}

/*!
  Returns the ORM object retrieved with the primary key \a pk from
  the table.
*/
template <class T>
inline T TSqlORMapper<T>::findByPrimaryKey(QVariant pk)
{
    int idx = T().primaryKeyIndex();
    if (idx < 0) {
        tSystemDebug("Primary key not found, table name: %s", qPrintable(T().tableName()));
        return T();
    }

    return findFirst(TCriteria(idx, pk));
}

/*!
  Retrieves with the criteria \a cri from the table and returns
  the number of the ORM objects. TSqlORMapperIterator is used to get
  the retrieved ORM objects.
  \sa TSqlORMapperIterator
*/
template <class T>
inline int TSqlORMapper<T>::find(const TCriteria &cri)
{
    if (!cri.isEmpty()) {
        TCriteriaConverter<T> conv(cri, database());
        setFilter(conv.toString());
    } else {
        setFilter(QString());
    }

    bool ret = select();
    tWriteQueryLog(query().lastQuery(), ret, lastError());
    tSystemDebug("rowCount: %d", rowCount());
    return ret ? rowCount() : -1;
}

/*!
  Retrieves with the criteria for the \a column as the \a value in the
  table and returns the number of the ORM objects. TSqlORMapperIterator
  is used to get the retrieved ORM objects.
  \sa TSqlORMapperIterator
*/
template <class T>
inline int TSqlORMapper<T>::findBy(int column, QVariant value)
{
    return find(TCriteria(column, value));
}

/*!
  Retrieves with the criteria that the \a column is within the list of values
  \a values returns the number of the ORM objects. TSqlORMapperIterator is
  used to get the retrieved ORM objects.
  \sa TSqlORMapperIterator
*/
template <class T>
inline int TSqlORMapper<T>::findIn(int column, const QVariantList &values)
{
    return find(TCriteria(column, TSql::In, values));
}

/*!
  Returns the first ORM object in the results retrieved by find() function.
  \sa find(const TCriteria &)
*/
template <class T>
inline T TSqlORMapper<T>::first() const
{
    return value(0);
}

/*!
  Returns the last ORM object in the results retrieved by find() function.
  \sa find(const TCriteria &)
*/
template <class T>
inline T TSqlORMapper<T>::last() const
{
    return value(rowCount() - 1);
}

/*!
  Returns the ORM object in the results retrieved by find() function.
  If \a i is the index of a valid row on the results, the ORM object
  will be populated with values from that row.
*/
template <class T>
inline T TSqlORMapper<T>::value(int i) const
{
    T rec;
    if (i >= 0 && i < rowCount()) {
        rec.setRecord(record(i), QSqlError());
    } else {
        tSystemDebug("no such record, index: %d  rowCount:%d", i, rowCount());
    }
    return rec;
}

/*!
  Sets the limit to \a limit, which is the limited number of rows for
  execution of SELECT statement.
*/
template <class T>
inline void TSqlORMapper<T>::setLimit(int limit)
{
    queryLimit = limit;
}

/*!
  Sets the offset to \a offset, which is the number of rows to skip
  for execution of SELECT statement.
*/
template <class T>
inline void TSqlORMapper<T>::setOffset(int offset)
{
    queryOffset = offset;
}

/*!
  Sets the sort order for \a column to \a order.
*/
template <class T>
inline void TSqlORMapper<T>::setSortOrder(int column, TSql::SortOrder order)
{
    sortColumn = column;
    sortOrder = (Tf::SortOrder)order;
}

/*!
  Sets the sort order for \a column to \a order.
*/
template <class T>
inline void TSqlORMapper<T>::setSortOrder(int column, Tf::SortOrder order)
{
    sortColumn = column;
    sortOrder = order;
}

/*!
  Sets the current filter to \a filter.
  The filter is a SQL WHERE clause without the keyword WHERE (for example,
  name='Hanako'). The filter will be applied the next time a query is
  executed.
*/
template <class T>
inline void TSqlORMapper<T>::setFilter(const QString &filter)
{
    queryFilter = filter;
}

/*!
  Returns a SELECT statement generated from the specified parameters.
  This function is for internal use only.
*/
template <class T>
inline QString TSqlORMapper<T>::selectStatement() const
{
    QString query = QSqlTableModel::selectStatement();

    if (!queryFilter.isEmpty())
        query.append(QLatin1String(" WHERE ")).append(queryFilter);

    QString orderby = orderBy();
    if (!orderby.isEmpty()) {
        query.append(orderby);
    }

    if (queryLimit > 0) {
        query.append(QLatin1String(" LIMIT ")).append(QString::number(queryLimit));
    }
    if (queryOffset > 0) {
        query.append(QLatin1String(" OFFSET ")).append(QString::number(queryOffset));
    }

    return query;
}

/*!
  Returns the number of records retrieved with the criteria \a cri
  from the table.
*/
template <class T>
inline int TSqlORMapper<T>::findCount(const TCriteria &cri)
{
    int cnt = -1;
    QString query = "SELECT COUNT(1) FROM ";
    query += tableName();

    if (!cri.isEmpty()) {
        TCriteriaConverter<T> conv(cri, database());
        query.append(QLatin1String(" WHERE ")).append(conv.toString());
    }

    TSqlQuery q(database());
    bool res = q.exec(query);
    if (res) {
        q.next();
        cnt = q.value(0).toInt();
    }
    return cnt;
}

/*!
  Returns the number of records retrieved with the criteria for the
  \a column as the \a value from the table.
*/
template <class T>
inline int TSqlORMapper<T>::findCountBy(int column, QVariant value)
{
    return findCount(TCriteria(column, value));
}

/*!
  Returns a list of all ORM objects in the results retrieved with the
  criteria \a cri from the table.
*/
template <class T>
inline QList<T> TSqlORMapper<T>::findAll(const TCriteria &cri)
{
    if (!cri.isEmpty()) {
        TCriteriaConverter<T> conv(cri);
        setFilter(conv.toString());
    } else {
        setFilter(QString());
    }

    QList<T> list;
    bool ret = select();
    tWriteQueryLog(query().lastQuery(), ret, lastError());

    if (ret) {
        tSystemDebug("rowCount: %d", rowCount());
        for (int i = 0; i < rowCount(); ++i) {
            T rec;
            rec.setRecord(record(i), QSqlError());
            list << rec;
        }
    }

    return list;
}

/*!
  Returns a list of all ORM objects in the results retrieved with the criteria
  for the \a column as the \a value.
*/
template <class T>
inline QList<T> TSqlORMapper<T>::findAllBy(int column, QVariant value)
{
    return findAll(TCriteria(column, value));
}

/*!
  Returns a list of all ORM objects in the results retrieved with the criteria
  that the \a column is within the list of values \a values.
*/
template <class T>
inline QList<T> TSqlORMapper<T>::findAllIn(int column, const QVariantList &values)
{
    return findAll(TCriteria(column, TSql::In, values));
}

/*!
  Updates the values of the columns specified in the first elements in the each pairs of \a values in
  all rows that satisfy the criteria \a cri and returns the number of the rows
  affected by the query executed.
*/
template <class T>
int TSqlORMapper<T>::updateAll(const TCriteria &cri, const QMap<int, QVariant> &values)
{
    QString upd;   // UPDATE Statement
    upd.reserve(256);
    upd.append(QLatin1String("UPDATE ")).append(tableName()).append(QLatin1String(" SET "));

    TCriteriaConverter<T> conv(cri, database());
    QString where = conv.toString();

    if (values.isEmpty()) {
        tSystemError("Update Parameter Error");
        return -1;
    }

    QMapIterator<int, QVariant> it(values);
    for (;;) {
        it.next();
        upd += TCriteriaConverter<T>::propertyName(it.key());
        upd += '=';
        upd += TSqlQuery::formatValue(it.value(), database());

        if (!it.hasNext())
            break;

        upd += QLatin1String(", ");
    }

    if (!where.isEmpty()) {
        upd.append(QLatin1String(" WHERE ")).append(where);
    }

    TSqlQuery sqlQuery(database());
    bool res = sqlQuery.exec(upd);
    return res ? sqlQuery.numRowsAffected() : -1;
}

/*!
  Updates the value of the specified \a column in all rows that satisfy the criteria
  \a cri and returns the number of the rows affected by the query executed.
*/
template <class T>
inline int TSqlORMapper<T>::updateAll(const TCriteria &cri, int column, QVariant value)
{
    QMap<int, QVariant> map;
    map.insert(column, value);
    return updateAll(cri, map);
}

/*!
  Removes all rows based on the criteria \a cri from the table and
  returns the number of the rows affected by the query executed.
*/
template <class T>
inline int TSqlORMapper<T>::removeAll(const TCriteria &cri)
{
    QString del = database().driver()->sqlStatement(QSqlDriver::DeleteStatement,
                                                    T().tableName(), QSqlRecord(), false);
    TCriteriaConverter<T> conv(cri, database());
    QString where = conv.toString();

    if (del.isEmpty()) {
        tSystemError("Statement Error");
        return -1;
    }
    if (!where.isEmpty()) {
        del.append(QLatin1String(" WHERE ")).append(where);
    }

    TSqlQuery sqlQuery(database());
    bool res = sqlQuery.exec(del);
    return res ? sqlQuery.numRowsAffected() : -1;
}

/*!
  Reset the internal state of the mapper object.
*/
template <class T>
inline void TSqlORMapper<T>::reset()
{
    setTable(tableName());
}

/*!
  Clears and releases any acquired resource.
*/
template <class T>
inline void TSqlORMapper<T>::clear()
{
    QSqlTableModel::clear();
    queryFilter.clear();
    sortColumn = -1;
    sortOrder = Tf::AscendingOrder;
    queryLimit = 0;
    queryOffset = 0;

    // Don't call the setTable() here,
    // or it causes a segmentation fault.
}

/*!
  Returns a SQL WHERE clause generated from a criteria.
*/
template <class T>
inline QString TSqlORMapper<T>::orderBy() const
{
    QString str;
    if (sortColumn >= 0) {
        QString field = TCriteriaConverter<T>::propertyName(sortColumn);
        if (!field.isEmpty()) {
            str.append(QLatin1String(" ORDER BY ")).append(field);
            str.append((sortOrder == Tf::AscendingOrder) ? QLatin1String(" ASC") : QLatin1String(" DESC"));
        }
    }
    return str;
}

#endif // TSQLORMAPPER_H
