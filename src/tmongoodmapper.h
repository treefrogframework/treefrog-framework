#ifndef TMONGOODMAPPER_H
#define TMONGOODMAPPER_H

#include <QVariant>
#include <TMongoQuery>
#include <TMongoObject>
#include <TCriteriaMongoConverter>
#include <TCriteria>

/*!
  \class TMongoODMapper
  \brief The TMongoODMapper class is a template class that provides
  concise functionality to object-document mapping for MongoDB.
  \sa TCriteria, TMongoQuery
*/


template <class T>
class TMongoODMapper : protected TMongoQuery
{
public:
    TMongoODMapper();
    virtual ~TMongoODMapper();

    void setLimit(int limit);
    void setOffset(int offset);
    void setSortOrder(int column, Tf::SortOrder order);

    T findOne(const TCriteria &cri = TCriteria());
    T findFirst(const TCriteria &cri = TCriteria()) { return findOne(cri); }
    T findFirstBy(int column, QVariant value);
    T findByObjectId(const QString &id);
    int find(const TCriteria &cri = TCriteria());
    int findBy(int column, QVariant value);
    int findIn(int column, const QVariantList &values);
    bool next();
    T value() const;

    int findCount(const TCriteria &cri = TCriteria());
    int findCountBy(int column, QVariant value);
    QList<T> findAll(const TCriteria &cri = TCriteria());
    QList<T> findAllBy(int column, QVariant value);
    QList<T> findAllIn(int column, const QVariantList &values);
    int updateAll(const TCriteria &cri, int column, QVariant value);
    int updateAll(const TCriteria &cri, const QMap<int, QVariant> &values);
    int removeAll(const TCriteria &cri = TCriteria());

private:
    int sortColumn;
    Tf::SortOrder sortOrder;
    Q_DISABLE_COPY(TMongoODMapper)
};


/*!
  Constructor.
*/
template <class T>
inline TMongoODMapper<T>::TMongoODMapper()
    : TMongoQuery(T().collectionName()), sortColumn(-1), sortOrder(Tf::AscendingOrder)
{ }

/*!
  Destructor.
*/
template <class T>
inline TMongoODMapper<T>::~TMongoODMapper()
{ }

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
void TMongoODMapper<T>::setSortOrder(int column, Tf::SortOrder order)
{
    sortColumn = column;
    sortOrder = order;
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
inline T TMongoODMapper<T>::findFirstBy(int column, QVariant value)
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
inline int TMongoODMapper<T>::find(const TCriteria &criteria)
{
    QVariantMap order;
    if (sortColumn >= 0) {
        QString name = TCriteriaMongoConverter<T>::propertyName(sortColumn);
        if (!name.isEmpty()) {
            order.insert(name, (sortOrder == Tf::AscendingOrder) ? 1 : -1);
        }
    }

    return TMongoQuery::find(TCriteriaMongoConverter<T>(criteria).toVariantMap(), order);
}


template <class T>
inline int TMongoODMapper<T>::findBy(int column, QVariant value)
{
    return find(TCriteria(column, value));
}


template <class T>
inline int TMongoODMapper<T>::findIn(int column, const QVariantList &values)
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
inline int TMongoODMapper<T>::findCountBy(int column, QVariant value)
{
    return findCount(TCriteria(column, value));
}


template <class T>
inline QList<T> TMongoODMapper<T>::findAll(const TCriteria &cri)
{
    QList<T> lst;
    int cnt = find(cri);
    tSystemDebug("Mongo documents count: %d", cnt);

    if (cnt > 0) {
        while (next()) {
            lst << value();
        }
    }
    return lst;
}


template <class T>
inline QList<T> TMongoODMapper<T>::findAllBy(int column, QVariant value)
{
    return findAll(TCriteria(column, value));
}


template <class T>
inline QList<T> TMongoODMapper<T>::findAllIn(int column, const QVariantList &values)
{
    return findAll(TCriteria(column, TMongo::In, values));
}


template <class T>
inline int TMongoODMapper<T>::updateAll(const TCriteria &cri, int column, QVariant value)
{
    QString s = TCriteriaMongoConverter<T>::propertyName(column);
    if (s.isEmpty())
        return -1;

    QVariantMap doc;
    doc.insert(s, value);
    bool res = TMongoQuery::updateMulti(TCriteriaMongoConverter<T>(cri).toVariantMap(), doc);
    return (res) ? numDocsAffected() : -1;
}


template <class T>
inline int TMongoODMapper<T>::updateAll(const TCriteria &cri, const QMap<int, QVariant> &values)
{
    QVariantMap doc;

    for (QMapIterator<int, QVariant> it(values); it.hasNext(); ) {
        it.next();
        QString s = TCriteriaMongoConverter<T>::propertyName(it.key());
        if (!s.isEmpty()) {
            doc.insert(s, it.value());
        }
    }

    bool res = TMongoQuery::updateMulti(TCriteriaMongoConverter<T>(cri).toVariantMap(), doc);
    return (res) ? numDocsAffected() : -1;
}


template <class T>
inline int TMongoODMapper<T>::removeAll(const TCriteria &criteria)
{
    return TMongoQuery::remove(TCriteriaMongoConverter<T>(criteria).toVariantMap());
}

#endif // TMONGOODMAPPER_H
