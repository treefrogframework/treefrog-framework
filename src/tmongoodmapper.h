#ifndef TMONGOODMAPPER_H
#define TMONGOODMAPPER_H

#include <QVariant>
#include <TMongoQuery>
#include <TMongoObject>
#include <TCriteriaMongoConverter>
#include <TCriteria>


template <class T>
class TMongoODMapper : protected TMongoQuery
{
public:
    TMongoODMapper();
    virtual ~TMongoODMapper();

    void setLimit(int limit);
    void setOffset(int offset);
    //void setSortOrder(int column, TSql::SortOrder order);

    T findOne(const TCriteria &cri = TCriteria());
    T findFirst(const TCriteria &cri = TCriteria()) { return findOne(cri); }
    T findByObjectId(const QString &id);
    int find(const TCriteria &cri = TCriteria());
    bool next();
    T value() const;
    int removeAll(const TCriteria &cri = TCriteria());

private:
    Q_DISABLE_COPY(TMongoODMapper)
};


/*!
  Constructor.
*/
template <class T>
inline TMongoODMapper<T>::TMongoODMapper()
    : TMongoQuery(T().collectionName())
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
inline T TMongoODMapper<T>::findOne(const TCriteria &criteria)
{
    T t;
    QVariantMap doc = TMongoQuery::findOne( TCriteriaMongoConverter<T>(criteria).toVariantMap() );
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
    return TMongoQuery::find( TCriteriaMongoConverter<T>(criteria).toVariantMap() );
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
inline int TMongoODMapper<T>::removeAll(const TCriteria &criteria)
{
    return TMongoQuery::remove( TCriteriaMongoConverter<T>(criteria).toVariantMap() );
}

#endif // TMONGOODMAPPER_H
