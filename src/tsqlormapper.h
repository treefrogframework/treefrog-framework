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
  functionality to object-relational mapping.
  \sa TSqlObject
*/


template <class T>
class TSqlORMapper : public QSqlTableModel
{
public:
    TSqlORMapper();
    virtual ~TSqlORMapper();

    void setLimit(int limit);
    void setOffset(int offset);
    void setSort(int column, TSql::SortOrder order);
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
    QString orderByPhrase() const;
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


template <class T>
inline T TSqlORMapper<T>::first() const
{
    return value(0);
}


template <class T>
inline T TSqlORMapper<T>::last() const
{
    return value(rowCount() - 1);
}


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


template <class T>
inline void TSqlORMapper<T>::setLimit(int limit)
{
    queryLimit = limit;
}


template <class T>
inline void TSqlORMapper<T>::setOffset(int offset)
{
    queryOffset = offset;
}


template <class T>
inline void TSqlORMapper<T>::setSort(int column, TSql::SortOrder order)
{
    sortColumn = column;
    sortOrder = order;
}


/*!
 * Sets the current filter to 'filter'.
 * The mapper doesn't re-selects it with the new filter,
 * the filter will be applied the next time select() is called.
 */
template <class T>
inline void TSqlORMapper<T>::setFilter(const QString &filter)
{
    queryFilter = filter;
}


template <class T>
inline QString TSqlORMapper<T>::selectStatement() const
{
    QString query = QSqlTableModel::selectStatement();
    if (!queryFilter.isEmpty())
        query.append(QLatin1String(" WHERE ")).append(queryFilter);

    QString orderby = orderByPhrase();
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


template <class T>
inline void TSqlORMapper<T>::reset()
{
    setTable(tableName());
}


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


template <class T>
inline QString TSqlORMapper<T>::orderByPhrase() const
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
