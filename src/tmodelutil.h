#ifndef TMODELUTIL_H
#define TMODELUTIL_H

#include <QList>
#include <TCriteria>
#include <TSqlORMapper>
#include <TSqlORMapperIterator>
#include <TMongoODMapper>


template <class T, class S>
inline QList<T> tfGetModelListByCriteria(const TCriteria &cri, int sortColumn, Tf::SortOrder order, int limit = 0, int offset = 0)
{
    TSqlORMapper<S> mapper;
    if (sortColumn >= 0)
        mapper.setSortOrder(sortColumn, order);

    if (limit > 0)
        mapper.setLimit(limit);

    if (offset > 0)
        mapper.setOffset(offset);

    QList<T> list;
    if (mapper.find(cri) > 0) {
        for (TSqlORMapperIterator<S> i(mapper); i.hasNext(); ) {
            list << T(i.next());
        }
    }
    return list;
}


template <class T, class S>
inline QList<T> tfGetModelListByCriteria(const TCriteria &cri = TCriteria(), int limit = 0, int offset = 0)
{
    return tfGetModelListByCriteria<T, S>(cri, -1, Tf::AscendingOrder, limit, offset);
}


template <class T, class S>
inline QList<T> tfGetModelListByMongoCriteria(const TCriteria &cri, int limit = 0, int offset = 0)
{
    TMongoODMapper<S> mapper;

    if (limit > 0)
        mapper.setLimit(limit);

    if (offset > 0)
        mapper.setOffset(offset);

    QList<T> list;
    if (mapper.find(cri) > 0) {
        while (mapper.next()) {
            list << T(mapper.value());
        }
    }
    return list;
}

#endif // TMODELUTIL_H
