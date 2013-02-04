#ifndef TSQLQUERYORMAPPER_H
#define TSQLQUERYORMAPPER_H

#include <QtSql>
#include <QList>
#include <TSqlQuery>
#include <TCriteriaConverter>
#include <TSystemGlobal>


template <class T>
class TSqlQueryORMapper : public TSqlQuery
{
public:
    TSqlQueryORMapper(const QString &query = QString(), int databaseId = 0);
    TSqlQueryORMapper(int databaseId);

    TSqlQueryORMapper<T> &prepare(const QString &query);
    bool load(const QString &filename);
    TSqlQueryORMapper<T> &bind(const QString &placeholder, const QVariant &val);
    TSqlQueryORMapper<T> &bind(int pos, const QVariant &val);
    TSqlQueryORMapper<T> &addBind(const QVariant &val);
    int find();
    T findFirst();
    T value() const;
    QString fieldName(int index) const;
};


template <class T>
inline TSqlQueryORMapper<T>::TSqlQueryORMapper(const QString &query, int databaseId)
    : TSqlQuery(query, databaseId)
{ }


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
    TSqlQuery::bind(val);
    return *this;
}


template <class T>
inline T TSqlQueryORMapper<T>::findFirst()
{
    exec();
    return (next()) ? value() : T();
}


// template <class T>
// inline QList<T> TSqlQueryORMapper<T>::findAll()
// {
//     exec();
//     QList<T> list;
//     while (next()) {
//         list.append(value());
//     }
//     return list;
// }


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
    return TCriteriaConverter<T>::propertyName(index);
}

#endif // TSQLQUERYORMAPPER_H
