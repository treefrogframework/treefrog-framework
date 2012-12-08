#ifndef MODELUTIL_H
#define MODELUTIL_H

#include <QList>
#include <TCriteria>
#include <TSqlORMapper>
#include <TSqlORMapperIterator>


template <class T, class S>
inline QList<T> tfGetModelListByCriteria(const TCriteria &cri, int sortColumn, TSql::SortOrder order, int limit = 0, int offset = 0)
{
    TSqlORMapper<S> mapper;
    if (sortColumn >= 0)
        mapper.setSort(sortColumn, order);

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
    return tfGetModelListByCriteria<T, S>(cri, -1, (TSql::SortOrder)0, limit, offset);
}

#endif // MODELUTIL_H
