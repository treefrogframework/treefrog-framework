#pragma once
#include <QVariant>
#include <TCriteria>
#include <TCriteriaMongoConverter>
#include <TMongoObject>
#include <TMongoQuery>

/*!
  \class TMongoODMapper
  \brief The TMongoODMapper class is a template class that provides
  concise functionality to object-document mapping for MongoDB.
  \sa TCriteria, TMongoQuery
*/


template <class T>
class TMongoODMapper : protected TMongoQuery {
public:
    TMongoODMapper();
    virtual ~TMongoODMapper();

    // Method chaining
    TMongoODMapper<T> &limit(int limit);
    TMongoODMapper<T> &offset(int offset);
    TMongoODMapper<T> &orderBy(int column, Tf::SortOrder order = Tf::AscendingOrder);
    TMongoODMapper<T> &orderBy(const QString &column, Tf::SortOrder order = Tf::AscendingOrder);

    void setLimit(int limit);
    void setOffset(int offset);
    void setSortOrder(int column, Tf::SortOrder order = Tf::AscendingOrder);
    void setSortOrder(const QString &column, Tf::SortOrder order = Tf::AscendingOrder);

    T findOne(const TCriteria &cri = TCriteria());
    T findFirst(const TCriteria &cri = TCriteria()) { return findOne(cri); }
    T findFirstBy(int column, const QVariant &value);
    T findByObjectId(const QString &id);
    bool find(const TCriteria &cri = TCriteria());
    bool findBy(int column, const QVariant &value);
    bool findIn(int column, const QVariantList &values);
    bool next();
    T value() const;

    int findCount(const TCriteria &cri = TCriteria());
    int findCountBy(int column, const QVariant &value);
    int updateAll(const TCriteria &cri, int column, const QVariant &value);
    int updateAll(const TCriteria &cri, const QMap<int, QVariant> &values);
    int removeAll(const TCriteria &cri = TCriteria());

private:
    QString sortColumn;
    Tf::SortOrder sortOrder;

    T_DISABLE_COPY(TMongoODMapper)
    T_DISABLE_MOVE(TMongoODMapper)
};


/*!
  Constructor.
*/
template <class T>
inline TMongoODMapper<T>::TMongoODMapper() :
    TMongoQuery(T().collectionName()), sortColumn(), sortOrder(Tf::AscendingOrder)
{
}

/*!
  Destructor.
*/
template <class T>
inline TMongoODMapper<T>::~TMongoODMapper()
{
}

template <class T>
inline void TMongoODMapper<T>::setLimit(int limit)
{
    TMongoQuery::setLimit(limit);
}


template <class T>
inline void TMongoODMapper<T>::setOffset(int offset)
{
    TMongoQuery::setOffset(offset);
}


template <class T>
inline void TMongoODMapper<T>::setSortOrder(int column, Tf::SortOrder order)
{
    if (column >= 0) {
        sortColumn = TCriteriaMongoConverter<T>::propertyName(column);
        sortOrder = order;
    }
}


template <class T>
inline void TMongoODMapper<T>::setSortOrder(const QString &column, Tf::SortOrder order)
{
    if (!column.isEmpty()) {
        T obj;
        if (obj.propertyNames().contains(column, Qt::CaseSensitive)) {
            sortColumn = column;
            sortOrder = order;
        } else {
            tWarn("Unable to set sort order : '%s' field not found in '%s' collection",
                qUtf8Printable(column), qUtf8Printable(obj.collectionName()));
        }
    }
}


template <class T>
TMongoODMapper<T> &TMongoODMapper<T>::limit(int l)
{
    setLimit(l);
    return *this;
}


template <class T>
inline TMongoODMapper<T> &TMongoODMapper<T>::offset(int o)
{
    setOffset(o);
    return *this;
}


template <class T>
inline TMongoODMapper<T> &TMongoODMapper<T>::orderBy(int column, Tf::SortOrder order)
{
    setSortOrder(column, order);
    return *this;
}


template <class T>
inline TMongoODMapper<T> &TMongoODMapper<T>::orderBy(const QString &column, Tf::SortOrder order)
{
    setSortOrder(column, order);
    return *this;
}


template <class T>
inline T TMongoODMapper<T>::findOne(const TCriteria &criteria)
{
    T t;
    QVariantMap doc = TMongoQuery::findOne(TCriteriaMongoConverter<T>(criteria).toVariantMap());
    if (!doc.isEmpty()) {
        t.setBsonData(doc);
    }
    return t;
}


template <class T>
inline T TMongoODMapper<T>::findFirstBy(int column, const QVariant &value)
{
    T t;
    TCriteria cri(column, value);
    QVariantMap doc = TMongoQuery::findOne(TCriteriaMongoConverter<T>(cri).toVariantMap());
    if (!doc.isEmpty()) {
        t.setBsonData(doc);
    }
    return t;
}


template <class T>
inline T TMongoODMapper<T>::findByObjectId(const QString &id)
{
    T t;
    QVariantMap doc = TMongoQuery::findById(id);
    if (!doc.isEmpty()) {
        t.setBsonData(doc);
    }
    return t;
}


template <class T>
inline bool TMongoODMapper<T>::find(const TCriteria &criteria)
{
    QVariantMap order;
    if (!sortColumn.isEmpty()) {
        order.insert(sortColumn, ((sortOrder == Tf::AscendingOrder) ? 1 : -1));
    }

    return TMongoQuery::find(TCriteriaMongoConverter<T>(criteria).toVariantMap(), order);
}


template <class T>
inline bool TMongoODMapper<T>::findBy(int column, const QVariant &value)
{
    return find(TCriteria(column, value));
}


template <class T>
inline bool TMongoODMapper<T>::findIn(int column, const QVariantList &values)
{
    return find(TCriteria(column, TMongo::In, values));
}


template <class T>
inline bool TMongoODMapper<T>::next()
{
    return TMongoQuery::next();
}


template <class T>
inline T TMongoODMapper<T>::value() const
{
    T t;
    QVariantMap doc = TMongoQuery::value();
    if (!doc.isEmpty()) {
        t.setBsonData(doc);
    }
    return t;
}


template <class T>
inline int TMongoODMapper<T>::findCount(const TCriteria &criteria)
{
    return TMongoQuery::count(TCriteriaMongoConverter<T>(criteria).toVariantMap());
}


template <class T>
inline int TMongoODMapper<T>::findCountBy(int column, const QVariant &value)
{
    return findCount(TCriteria(column, value));
}


// template <class T>
// inline QList<T> TMongoODMapper<T>::findAll(const TCriteria &cri)
// {
//     QList<T> lst;
//     int cnt = find(cri);
//     tSystemDebug("Mongo documents count: %d", cnt);

//     if (cnt > 0) {
//         while (next()) {
//             lst << value();
//         }
//     }
//     return lst;
// }


// template <class T>
// inline QList<T> TMongoODMapper<T>::findAllBy(int column, QVariant value)
// {
//     return findAll(TCriteria(column, value));
// }


// template <class T>
// inline QList<T> TMongoODMapper<T>::findAllIn(int column, const QVariantList &values)
// {
//     return findAll(TCriteria(column, TMongo::In, values));
// }


template <class T>
inline int TMongoODMapper<T>::updateAll(const TCriteria &cri, int column, const QVariant &value)
{
    QString s = TCriteriaMongoConverter<T>::propertyName(column);
    if (s.isEmpty())
        return -1;

    QVariantMap doc;
    doc.insert(s, value);
    return TMongoQuery::updateMulti(TCriteriaMongoConverter<T>(cri).toVariantMap(), doc);
}


template <class T>
inline int TMongoODMapper<T>::updateAll(const TCriteria &cri, const QMap<int, QVariant> &values)
{
    QVariantMap doc;

    for (auto it = values.begin(); it != values.end(); ++it) {
        QString s = TCriteriaMongoConverter<T>::propertyName(it.key());
        if (!s.isEmpty()) {
            doc.insert(s, it.value());
        }
    }

    return TMongoQuery::updateMulti(TCriteriaMongoConverter<T>(cri).toVariantMap(), doc);
}


template <class T>
inline int TMongoODMapper<T>::removeAll(const TCriteria &criteria)
{
    return TMongoQuery::remove(TCriteriaMongoConverter<T>(criteria).toVariantMap());
}

