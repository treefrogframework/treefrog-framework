#ifndef TSQLORMAPPER_H
#define TSQLORMAPPER_H

#include <QtSql>
#include <QList>
#include <TSqlObject>
#include <TCriteria>
#include <TCriteriaConverter>
#include <TActionContext>
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
    void setSortOrder(int column, TSql::SortOrder order);
    void reset();

    T findFirst(const TCriteria &cri = TCriteria());
    T findByPrimaryKey(QVariant pk);
    int find(const TCriteria &cri = TCriteria());
    T first() const;
    T last() const;
    T value(int i) const;
    int removeAll(const TCriteria &cri = TCriteria());

protected:
    void setFilter(const QString &filter);
    QString orderByClause() const;
    virtual void clear();
    virtual QString selectStatement() const;

private:
    Q_DISABLE_COPY(TSqlORMapper)

    QString queryFilter;
    int sortColumn;
    TSql::SortOrder sortOrder;
    int queryLimit;
    int queryOffset;
};


/*!
  Constructor.
*/
template <class T>
inline TSqlORMapper<T>::TSqlORMapper()
    : QSqlTableModel(0, TActionContext::current()->getDatabase(T().databaseId())),
      sortColumn(-1), sortOrder(TSql::AscendingOrder), queryLimit(0),
      queryOffset(0)
{
    setTable(T().tableName());
}


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
    }

    int oldLimit = queryLimit;
    queryLimit = 1;
    select();
    queryLimit = oldLimit;

    tSystemDebug("rowCount: %d", rowCount());
    return first();
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

    TCriteria cri(idx, pk);
    TCriteriaConverter<T> conv(cri, database());
    setFilter(conv.toString());
    select();
    tSystemDebug("findByPrimaryKey() rowCount: %d", rowCount());
    return first();
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
    }
    if (!select()) {
        return -1;
    }
    tSystemDebug("rowCount: %d", rowCount());
    return rowCount();
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
  Sets the limit to \a limit, which is the limited number of rows.
*/
template <class T>
inline void TSqlORMapper<T>::setLimit(int limit)
{
    queryLimit = limit;
}

/*!
  Sets the offset to \a offset, which is the number of rows to skip.
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

    QString orderby = orderByClause();
    if (!orderby.isEmpty()) {
        query.append(orderby);
    }

    if (queryLimit > 0) {
        query.append(QLatin1String(" LIMIT ")).append(QString::number(queryLimit));
    }
    if (queryOffset > 0) {
        query.append(QLatin1String(" OFFSET ")).append(QString::number(queryOffset));
    }
    tQueryLog("%s", qPrintable(query));
    return query;
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

    tQueryLog("%s", qPrintable(del));
  
    QSqlQuery sqlQuery(database());
    if ( !sqlQuery.exec(del) ) {
        return -1;
    }
    return sqlQuery.numRowsAffected();
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
    sortOrder = TSql::AscendingOrder;
    queryLimit = 0;
    queryOffset = 0;
    
    // Don't call the setTable() here,
    // or it causes a segmentation fault.
}

/*!
  Returns a SQL WHERE clause generated from a criteria.
*/
template <class T>
inline QString TSqlORMapper<T>::orderByClause() const
{
    QString str;
    if (sortColumn >= 0) {
        QString f = TCriteriaConverter<T>::propertyName(sortColumn);
        if (!f.isEmpty()) {
            QString field = TSqlQuery::escapeIdentifier(f, QSqlDriver::FieldName, database());
            str.append(QLatin1String(" ORDER BY ")).append(field);
            str.append((sortOrder == TSql::AscendingOrder) ? QLatin1String(" ASC") : QLatin1String(" DESC"));
        }
    }
    return str;
}

#endif // TSQLORMAPPER_H
