#ifndef TSQLQUERYORMAPPER_H
#define TSQLQUERYORMAPPER_H

#include <QtSql>
#include <QList>
#include <TSqlQuery>
#include <TCriteriaConverter>
#include <TSystemGlobal>

/*!
  \class TSqlQueryORMapper
  \brief The TSqlQueryORMapper class is a template class that provides
  concise functionality to object-relational mapping by executing SQL
  statements. It can be used to retrieve TSqlObject objects with SQL query
  from a table.
  \sa TSqlQuery, TSqlObject
*/

template <class T>
class TSqlQueryORMapper : public TSqlQuery
{
public:
    TSqlQueryORMapper(int databaseId = 0);

    TSqlQueryORMapper<T> &prepare(const QString &query);
    bool load(const QString &filename);
    TSqlQueryORMapper<T> &bind(const QString &placeholder, const QVariant &val);
    TSqlQueryORMapper<T> &bind(int pos, const QVariant &val);
    TSqlQueryORMapper<T> &addBind(const QVariant &val);
    bool exec(const QString &query);
    bool exec();
    T execFirst(const QString &query);
    T execFirst();
    QList<T> execAll();
    int numRowsAffected() const;
    int size() const;
    bool next();
    T value() const;
    QString fieldName(int index) const;
};


template <class T>
inline TSqlQueryORMapper<T>::TSqlQueryORMapper(int databaseId)
    : TSqlQuery(databaseId)
{ }


template <class T>
inline TSqlQueryORMapper<T> &TSqlQueryORMapper<T>::prepare(const QString &query)
{
    TSqlQuery::prepare(query);
    return *this;
}


template <class T>
inline bool TSqlQueryORMapper<T>::load(const QString &filename)
{
    return TSqlQuery::load(filename);
}


template <class T>
inline TSqlQueryORMapper<T> &TSqlQueryORMapper<T>::bind(const QString &placeholder, const QVariant &val)
{
    TSqlQuery::bind(placeholder, val);
    return *this;
}


template <class T>
inline TSqlQueryORMapper<T> &TSqlQueryORMapper<T>::bind(int pos, const QVariant &val)
{
    TSqlQuery::bind(pos, val);
    return *this;
}


template <class T>
inline TSqlQueryORMapper<T> &TSqlQueryORMapper<T>::addBind(const QVariant &val)
{
    TSqlQuery::addBind(val);
    return *this;
}


template <class T>
inline bool TSqlQueryORMapper<T>::exec(const QString &query)
{
    return TSqlQuery::exec(query);
}


template <class T>
inline bool TSqlQueryORMapper<T>::exec()
{
    return TSqlQuery::exec();
}


template <class T>
inline T TSqlQueryORMapper<T>::execFirst(const QString &query)
{
    return (exec(query) && next()) ? value() : T();
}


template <class T>
inline T TSqlQueryORMapper<T>::execFirst()
{
    return (exec() && next()) ? value() : T();
}


template <class T>
inline QList<T> TSqlQueryORMapper<T>::execAll()
{
    QList<T> ret;

    if (exec()) {
        while (next()) {
            ret << value();
        }
    }
    return ret;
}


template <class T>
inline int TSqlQueryORMapper<T>::numRowsAffected() const
{
    return TSqlQuery::numRowsAffected();
}


template <class T>
inline int TSqlQueryORMapper<T>::size() const
{
    return TSqlQuery::size();
}


template <class T>
inline bool TSqlQueryORMapper<T>::next()
{
    return TSqlQuery::next();
}


template <class T>
inline T TSqlQueryORMapper<T>::value() const
{
    T rec;
    QSqlRecord r = record();
    rec.setRecord(r, lastError());
    return rec;
}


template <class T>
inline QString TSqlQueryORMapper<T>::fieldName(int index) const
{
    return TCriteriaConverter<T>::propertyName(index, driver());
}

#endif // TSQLQUERYORMAPPER_H
