#ifndef TSQLQUERYITERATOR_H
#define TSQLQUERYITERATOR_H

#include <TSqlQuery>


template <class T>
class TSqlQueryIterator
{
public:
    TSqlQueryIterator(const TSqlQueryIterator<T> &iterator);
    TSqlQueryIterator<T> &operator=(const TSqlQueryIterator<T> &iterator);

    bool hasNext() const;
    bool hasPrevious() const;
    T next();
    T previous();
    bool toBack();
    bool toFront();
    T value() const;

private:
    TSqlQueryIterator(TSqlQuery<T> *query);
    TSqlQuery<T> *sqlQuery;
  
    friend class TSqlQuery<T>;
};


template <class T>
inline TSqlQueryIterator<T>::TSqlQueryIterator(const TSqlQueryIterator<T> &iterator)
    : sqlQuery(iterator.sqlQuery)
{ }

template <class T>
inline TSqlQueryIterator<T> &TSqlQueryIterator<T>::operator=(const TSqlQueryIterator<T> &iterator)
{
    sqlQuery = iterator.sqlQuery;
    return *this;
}

template <class T>
inline TSqlQueryIterator<T>::TSqlQueryIterator(TSqlQuery<T> *query)
    : sqlQuery(query)
{ }

template <class T>
inline bool TSqlQueryIterator<T>::hasNext() const
{
    if (!sqlQuery || !sqlQuery->isSelect() || !sqlQuery->isActive())
        return false;

    if (sqlQuery->at() == QSql::BeforeFirstRow) {
        return (sqlQuery->size() > 0);
    } else if (sqlQuery->at() == QSql::AfterLastRow) {
        return false;
    } else {
        return (sqlQuery->at() + 1 < sqlQuery->size());
    }
}

template <class T>
inline bool TSqlQueryIterator<T>::hasPrevious() const
{
    if (!sqlQuery || !sqlQuery->isSelect() || !sqlQuery->isActive())
        return false;
  
    if (sqlQuery->at() == QSql::BeforeFirstRow) {
        return false;
    } else if (sqlQuery->at() == QSql::AfterLastRow) {
        return (sqlQuery->size() > 0);
    } else {
        return (sqlQuery->at() > 0);
    }
}

template <class T>
inline T TSqlQueryIterator<T>::next()
{
    if (!sqlQuery)
        return T();
  
    if (!sqlQuery->next()) {
        tSystemDebug("no such record, at: {}  size: {}", sqlQuery->at(), sqlQuery->size());
        return T();
    }
    return value();
}

template <class T>
inline T TSqlQueryIterator<T>::previous()
{
    if (!sqlQuery)
        return T();

    if (!sqlQuery->previous()) {
        tSystemDebug("no such record, at: {}  size: {}", sqlQuery->at(), sqlQuery->size());
        return T();
    }
    return value();
}

template <class T>
inline bool TSqlQueryIterator<T>::toBack()
{
    if (!sqlQuery)
        return false;

    return sqlQuery->last();
}


template <class T>
inline bool TSqlQueryIterator<T>::toFront()
{
    if (!sqlQuery)
        return false;

    return sqlQuery->first();
}

template <class T>
inline T TSqlQueryIterator<T>::value() const
{
    T rec;
    if (!sqlQuery)
        return rec;
  
    QSqlRecord r = sqlQuery->record();
    rec.setRecord(r, sqlQuery->lastError());
    return rec;
}

#endif // TSQLQUERYITERATOR_H
