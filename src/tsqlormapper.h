#pragma once
#include "tsystemglobal.h"
#include <QList>
#include <QMap>
#include <QtSql>
#include <TCriteria>
#include <TCriteriaConverter>
#include <TGlobal>
#include <TSqlJoin>
#include <TSqlObject>
#include <TSqlQuery>

/*!
  \class TAbstractSqlORMapper
  \brief The TAbstractSqlORMapper class is the abstract base class of
  functionality to object-relational mapping.
  \sa TSqlORMapper
*/

class TAbstractSqlORMapper : public QSqlTableModel {
public:
    TAbstractSqlORMapper(const QSqlDatabase &db) : QSqlTableModel(nullptr, db) {}
    virtual ~TAbstractSqlORMapper() {}

    virtual void setLimit(int limit) = 0;
    virtual void setOffset(int offset) = 0;
    virtual void setSortOrder(int column, Tf::SortOrder order = Tf::AscendingOrder) = 0;
    virtual void setSortOrder(const QString &column, Tf::SortOrder order = Tf::AscendingOrder) = 0;
    virtual void reset() = 0;

    virtual int find(const TCriteria &cri = TCriteria()) = 0;
    virtual int findBy(int column, const QVariant &value) = 0;
    virtual int findIn(int column, const QVariantList &values) = 0;

    virtual int findCount(const TCriteria &cri = TCriteria()) = 0;
    virtual int findCountBy(int column, const QVariant &value) = 0;
    virtual int updateAll(const TCriteria &cri, int column, const QVariant &value) = 0;
    virtual int updateAll(const TCriteria &cri, const QMap<int, QVariant> &values) = 0;
    virtual int removeAll(const TCriteria &cri = TCriteria()) = 0;
};


/*!
  \class TSqlORMapper
  \brief The TSqlORMapper class is a template class that provides
  concise functionality to object-relational mapping.
  It can be used to retrieve TSqlObject objects with a TCriteria
  from a table.
  \sa TSqlObject, TCriteria
*/

template <class T>
class TSqlORMapper : public TAbstractSqlORMapper {
public:
    TSqlORMapper();
    virtual ~TSqlORMapper();

    // Method chaining
    TSqlORMapper<T> &limit(int limit);
    TSqlORMapper<T> &offset(int offset);
    TSqlORMapper<T> &orderBy(int column, Tf::SortOrder order = Tf::AscendingOrder);
    TSqlORMapper<T> &orderBy(const QString &column, Tf::SortOrder order = Tf::AscendingOrder);
    template <class C>
    TSqlORMapper<T> &join(int column, const TSqlJoin<C> &join);

    void setLimit(int limit);
    void setOffset(int offset);
    void setSortOrder(int column, Tf::SortOrder order = Tf::AscendingOrder);
    void setSortOrder(const QString &column, Tf::SortOrder order = Tf::AscendingOrder);
    template <class C>
    void setJoin(int column, const TSqlJoin<C> &join);
    void reset();

    T findFirst(const TCriteria &cri = TCriteria());
    T findFirstBy(int column, const QVariant &value);
    T findByPrimaryKey(const QVariant &pk);
    int find(const TCriteria &cri = TCriteria());
    int findBy(int column, const QVariant &value);
    int findIn(int column, const QVariantList &values);
    int rowCount() const;
    T first() const;
    T last() const;
    T value(int i) const;

    int findCount(const TCriteria &cri = TCriteria());
    int findCountBy(int column, const QVariant &value);
    int updateAll(const TCriteria &cri, int column, const QVariant &value);
    int updateAll(const TCriteria &cri, const QMap<int, QVariant> &values);
    int removeAll(const TCriteria &cri = TCriteria());

    class ConstIterator;
    inline ConstIterator begin() const { return ConstIterator(this, 0); }
    inline ConstIterator end() const { return ConstIterator(this, rowCount()); }

    /*!
      Const iterator
     */
    class ConstIterator {
    public:
        const TSqlORMapper<T> *m {nullptr};
        int it {0};

        inline ConstIterator() {}
        inline ConstIterator(const ConstIterator &o) :
            m(o.m), it(o.it) {}
        inline ConstIterator &operator=(const ConstIterator &o)
        {
            m = o.m;
            it = o.it;
            return *this;
        }
        inline const T operator*() const { return m->value(it); }
        inline bool operator==(const ConstIterator &o) const { return m == o.m && it == o.it; }
        inline bool operator!=(const ConstIterator &o) const { return m != o.m || it != o.it; }
        inline ConstIterator &operator++()
        {
            it = qMin(it + 1, m->rowCount());
            return *this;
        }
        inline ConstIterator operator++(int)
        {
            int i = it;
            it = qMin(it + 1, m->rowCount());
            return ConstIterator(m, i);
        }
        inline ConstIterator &operator--()
        {
            --it;
            return *this;
        }
        inline ConstIterator operator--(int)
        {
            int i = it++;
            return ConstIterator(m, i);
        }

    private:
        inline ConstIterator(const TSqlORMapper<T> *mapper, int i) :
            m(mapper), it(i) {}
        friend class TSqlORMapper;
    };

protected:
    void setFilter(const QString &filter);
    QString orderBy() const;
    virtual QString orderByClause() const { return QString(); }
    virtual void clear();
    virtual QString selectStatement() const;
    virtual int rowCount(const QModelIndex &parent) const;

private:
    QString queryFilter;
    QList<QPair<QString, Tf::SortOrder>> sortColumns;
    int queryLimit {0};
    int queryOffset {0};
    int joinCount {0};
    QStringList joinClauses;
    QStringList joinWhereClauses;

    T_DISABLE_COPY(TSqlORMapper)
    T_DISABLE_MOVE(TSqlORMapper)
};

/*!
  Constructor.
*/
template <class T>
inline TSqlORMapper<T>::TSqlORMapper() :
    TAbstractSqlORMapper(Tf::currentSqlDatabase(T().databaseId()))
{
    setTable(T().tableName());
}

/*!
  Destructor.
*/
template <class T>
inline TSqlORMapper<T>::~TSqlORMapper()
{
}

/*!
  Returns the first ORM object retrieved with the criteria \a cri from
  the table.
*/
template <class T>
inline T TSqlORMapper<T>::findFirst(const TCriteria &cri)
{
    if (!cri.isEmpty()) {
        TCriteriaConverter<T> conv(cri, database(), QStringLiteral("t0"));
        setFilter(conv.toString());
    } else {
        setFilter(QString());
    }

    int oldLimit = queryLimit;
    queryLimit = 1;
    bool ret = select();
    Tf::writeQueryLog(query().lastQuery(), ret, lastError());
    queryLimit = oldLimit;

    //tSystemDebug("findFirst() rowCount: %d", rowCount());
    return first();
}

/*!
  Returns the first ORM object retrieved with the criteria for the
  \a column as the \a value in the table.
*/
template <class T>
inline T TSqlORMapper<T>::findFirstBy(int column, const QVariant &value)
{
    return findFirst(TCriteria(column, value));
}

/*!
  Returns the ORM object retrieved with the primary key \a pk from
  the table.
*/
template <class T>
inline T TSqlORMapper<T>::findByPrimaryKey(const QVariant &pk)
{
    int idx = T().primaryKeyIndex();
    if (idx < 0) {
        tSystemDebug("Primary key not found, table name: %s", qUtf8Printable(T().tableName()));
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
        TCriteriaConverter<T> conv(cri, database(), QStringLiteral("t0"));
        setFilter(conv.toString());
    } else {
        setFilter(QString());
    }

    bool ret = select();
    while (canFetchMore()) {  // For SQLite, not report back the size of a query
        fetchMore();
    }
    Tf::writeQueryLog(query().lastQuery(), ret, lastError());
    //tSystemDebug("find() rowCount: %d", rowCount());
    return ret ? rowCount() : -1;
}

/*!
  Retrieves with the criteria for the \a column as the \a value in the
  table and returns the number of the ORM objects. TSqlORMapperIterator
  is used to get the retrieved ORM objects.
  \sa TSqlORMapperIterator
*/
template <class T>
inline int TSqlORMapper<T>::findBy(int column, const QVariant &value)
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
  Returns the number of rows of the current query.
 */
template <class T>
inline int TSqlORMapper<T>::rowCount() const
{
    return QSqlTableModel::rowCount();
}

/*!
  Returns the number of rows of the current query.
 */
template <class T>
inline int TSqlORMapper<T>::rowCount(const QModelIndex &parent) const
{
    return QSqlTableModel::rowCount(parent);
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
inline void TSqlORMapper<T>::setSortOrder(int column, Tf::SortOrder order)
{
    if (column >= 0) {
        QString columnName = TCriteriaConverter<T>::getPropertyName(column, QSqlTableModel::database().driver());
        if (!columnName.isEmpty()) {
            sortColumns << qMakePair(columnName, order);
        }
    }
}

/*!
  Sets the sort order for \a column to \a order.
*/
template <class T>
inline void TSqlORMapper<T>::setSortOrder(const QString &column, Tf::SortOrder order)
{
    if (!column.isEmpty()) {
        T obj;
        if (obj.propertyNames().contains(column, Qt::CaseInsensitive)) {
            sortColumns << qMakePair(column, order);
        } else {
            tWarn("Unable to set sort order : '%s' column not found in '%s' table",
                qUtf8Printable(column), qUtf8Printable(obj.tableName()));
        }
    }
}

/*!
  Sets the limit to \a limit, which is the limited number of rows for
  execution of SELECT statement.
*/
template <class T>
inline TSqlORMapper<T> &TSqlORMapper<T>::limit(int l)
{
    setLimit(l);
    return *this;
}

/*!
  Sets the offset to \a offset, which is the number of rows to skip
  for execution of SELECT statement.
*/
template <class T>
inline TSqlORMapper<T> &TSqlORMapper<T>::offset(int o)
{
    setOffset(o);
    return *this;
}

/*!
  Sets the sort order for \a column to \a order.
*/
template <class T>
inline TSqlORMapper<T> &TSqlORMapper<T>::orderBy(int column, Tf::SortOrder order)
{
    setSortOrder(column, order);
    return *this;
}

/*!
  Sets the sort order for \a column to \a order.
*/
template <class T>
inline TSqlORMapper<T> &TSqlORMapper<T>::orderBy(const QString &column, Tf::SortOrder order)
{
    setSortOrder(column, order);
    return *this;
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
    QString query;
    bool joinFlag = !joinClauses.isEmpty();
    bool valid = false;
    auto rec = record();

    query.reserve(1024);
    query += QLatin1String("SELECT ");

    for (int i = 0; i < rec.count(); ++i) {
        if (rec.isGenerated(i)) {
            if (joinFlag) {
                query += QLatin1String("t0.");
            }
            query += TSqlQuery::escapeIdentifier(rec.fieldName(i), QSqlDriver::FieldName, database().driver());
            query += QLatin1Char(',');
            valid = true;
        }
    }

    if (Q_UNLIKELY(!valid)) {
        return QString();
    }

    query.chop(1);
    query += QLatin1String(" FROM ");
    query += TSqlQuery::escapeIdentifier(tableName(), QSqlDriver::TableName, database().driver());
    query += QLatin1String(" t0");  // alias needed

    if (joinFlag) {
        for (auto &join : (const QStringList &)joinClauses) {
            query += join;
        }
    }

    QString filter = queryFilter;
    if (!joinWhereClauses.isEmpty()) {
        for (auto &wh : (const QStringList &)joinWhereClauses) {
            if (!filter.isEmpty()) {
                filter += QLatin1String(" AND ");
            }
            filter += wh;
        }
    }

    if (Q_LIKELY(!filter.isEmpty())) {
        query.append(QLatin1String(" WHERE ")).append(filter);
    }

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
    if (!cri.isEmpty()) {
        TCriteriaConverter<T> conv(cri, database(), "t0");
        setFilter(conv.toString());
    } else {
        setFilter(QString());
    }

    QString query;
    query.reserve(1024);
    query += QLatin1String("SELECT COUNT(*) FROM (");
    query += selectStatement();
    query += QLatin1String(") t");

    int cnt = -1;
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
inline int TSqlORMapper<T>::findCountBy(int column, const QVariant &value)
{
    return findCount(TCriteria(column, value));
}

/*!
  Updates the values of the columns specified in the first elements in the each pairs of \a values in
  all rows that satisfy the criteria \a cri and returns the number of the rows
  affected by the query executed.
*/
template <class T>
int TSqlORMapper<T>::updateAll(const TCriteria &cri, const QMap<int, QVariant> &values)
{
    static const QByteArray UpdatedAt("updated_at");
    static const QByteArray ModifiedAt("modified_at");

    QString upd;  // UPDATE Statement
    upd.reserve(256);
    upd.append(QLatin1String("UPDATE ")).append(tableName()).append(QLatin1String(" SET "));

    QSqlDatabase db = database();
    TCriteriaConverter<T> conv(cri, db);
    QString where = conv.toString();

    if (values.isEmpty()) {
        tSystemError("Update Parameter Error");
        return -1;
    }

    T obj;
    for (int i = obj.metaObject()->propertyOffset(); i < obj.metaObject()->propertyCount(); ++i) {
        const char *propName = obj.metaObject()->property(i).name();
        QByteArray prop = QByteArray(propName).toLower();
        if (prop == UpdatedAt || prop == ModifiedAt) {
            upd += propName;
            upd += QLatin1Char('=');
#if QT_VERSION < 0x060000
            constexpr auto metaType = QVariant::DateTime;
#else
            static const QMetaType metaType(QMetaType::QDateTime);
#endif
            upd += TSqlQuery::formatValue(QDateTime::currentDateTime(), metaType, db);
            upd += QLatin1Char(',');
            break;
        }
    }

    auto it = values.begin();
    while (true) {
        upd += conv.propertyName(it.key(), db.driver());
        upd += QLatin1Char('=');
        upd += TSqlQuery::formatValue(it.value(), conv.variantType(it.key()), db);

        if (++it == values.end()) {
            break;
        }
        upd += QLatin1Char(',');
    }

    if (!where.isEmpty()) {
        upd.append(QLatin1String(" WHERE ")).append(where);
    }

    TSqlQuery sqlQuery(db);
    bool res = sqlQuery.exec(upd);
    return res ? sqlQuery.numRowsAffected() : -1;
}

/*!
  Updates the value of the specified \a column in all rows that satisfy the criteria
  \a cri and returns the number of the rows affected by the query executed.
*/
template <class T>
inline int TSqlORMapper<T>::updateAll(const TCriteria &cri, int column, const QVariant &value)
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
    QSqlDatabase db = database();
    QString del = db.driver()->sqlStatement(QSqlDriver::DeleteStatement,
        T().tableName(), QSqlRecord(), false);
    TCriteriaConverter<T> conv(cri, db);
    QString where = conv.toString();

    if (del.isEmpty()) {
        tSystemError("Statement Error");
        return -1;
    }
    if (!where.isEmpty()) {
        del.append(QLatin1String(" WHERE ")).append(where);
    }

    TSqlQuery sqlQuery(db);
    bool res = sqlQuery.exec(del);
    return res ? sqlQuery.numRowsAffected() : -1;
}

/*!
  Sets a JOIN clause for \a column to \a join.
 */
template <class T>
template <class C>
inline void TSqlORMapper<T>::setJoin(int column, const TSqlJoin<C> &join)
{
    if (column < 0 || join.joinColumn() < 0) {
        return;
    }

    QString clause;

    switch (join.joinMode()) {
    case TSql::InnerJoin:
        clause = QLatin1String(" INNER JOIN ");
        break;

    case TSql::LeftJoin:
        clause = QLatin1String(" LEFT OUTER JOIN ");
        break;

    case TSql::RightJoin:
        clause = QLatin1String(" RIGHT OUTER JOIN ");
        break;

    default:
        break;
    }

    int joinCount = joinClauses.count();
    QString alias = QLatin1Char('t') + QString::number(joinCount + 1);
    QSqlDatabase db = database();

    clause += C().tableName();
    clause += QLatin1Char(' ');
    clause += alias;
    clause += QLatin1String(" ON ");
    clause += TCriteriaConverter<T>::getPropertyName(column, db.driver(), "t0");
    clause += QLatin1Char('=');
    clause += TCriteriaConverter<C>::getPropertyName(join.joinColumn(), db.driver(), alias);
    joinClauses << clause;

    if (!join.criteria().isEmpty()) {
        TCriteriaConverter<C> conv(join.criteria(), db, alias);
        joinWhereClauses << conv.toString();
    }
}

template <class T>
template <class C>
inline TSqlORMapper<T> &TSqlORMapper<T>::join(int column, const TSqlJoin<C> &j)
{
    setJoin(column, j);
    return *this;
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
    sortColumns.clear();
    queryLimit = 0;
    queryOffset = 0;
    joinCount = 0;
    joinClauses.clear();
    joinWhereClauses.clear();

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

    if (!sortColumns.isEmpty()) {
        str.reserve(64);
        str += QLatin1String(" ORDER BY ");
        for (auto &p : sortColumns) {
            str += QLatin1String("t0.");
            str += TSqlQuery::escapeIdentifier(p.first, QSqlDriver::FieldName, database().driver());
            str += (p.second == Tf::AscendingOrder) ? QLatin1String(" ASC,") : QLatin1String(" DESC,");
        }
        str.chop(1);
    }
    return str;
}

