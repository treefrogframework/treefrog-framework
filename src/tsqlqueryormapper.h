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
    TSqlQueryORMapper(const QString &query = QString());

    T findFirst();
    QList<T> findAll();
    T value() const;
    QString fieldName(int index) const;
};


template <class T>
inline TSqlQueryORMapper<T>::TSqlQueryORMapper(const QString &query)
    : TSqlQuery(query)
{ }


template <class T>
inline T TSqlQueryORMapper<T>::findFirst()
{
    exec();
    return (next()) ? value() : T();
}


template <class T>
inline QList<T> TSqlQueryORMapper<T>::findAll()
{
    exec();
    QList<T> list;
    while (next()) {
        list.append(value());
    }
    return list;
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
    return TCriteriaConverter<T>::propertyName(index);
}

#endif // TSQLQUERYORMAPPER_H
