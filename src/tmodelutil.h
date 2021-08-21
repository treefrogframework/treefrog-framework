#pragma once
#include <QList>
#include <TCriteria>
#include <TCriteriaConverter>
#include <TMongoODMapper>
#include <TSqlORMapper>
#include <TSqlORMapperIterator>


template <class T, class S>
inline QList<T> tfGetModelListByCriteria(const TCriteria &cri, const QList<QPair<QString, Tf::SortOrder>> &sortColumns, int limit = 0, int offset = 0)
{
    TSqlORMapper<S> mapper;
    if (!sortColumns.isEmpty()) {
        for (auto &p : sortColumns) {
            if (!p.first.isEmpty()) {
                mapper.setSortOrder(p.first, p.second);
            }
        }
    }
    if (limit > 0) {
        mapper.setLimit(limit);
    }
    if (offset > 0) {
        mapper.setOffset(offset);
    }
    QList<T> list;
    int count = mapper.find(cri);
    if (count > 0) {
        list.reserve(count);
        for (auto &o : mapper) {
            list << T(o);
        }
    }
    return list;
}

template <class T, class S>
inline QList<T> tfGetModelListByCriteria(const TCriteria &cri, const QList<QPair<int, Tf::SortOrder>> &sortColumns, int limit = 0, int offset = 0)
{
    QList<QPair<QString, Tf::SortOrder>> sorts;

    for (auto &p : sortColumns) {
        QString columnName = TCriteriaConverter<S>::getPropertyName(p.first, nullptr);
        if (!columnName.isEmpty()) {
            sorts << qMakePair(columnName, p.second);
        }
    }
    return tfGetModelListByCriteria<T, S>(cri, sorts, limit, offset);
}

template <class T, class S>
inline QList<T> tfGetModelListByCriteria(const TCriteria &cri, int sortColumn, Tf::SortOrder order, int limit = 0, int offset = 0)
{
    QList<QPair<int, Tf::SortOrder>> sortColumns = {qMakePair(sortColumn, order)};
    return tfGetModelListByCriteria<T, S>(cri, sortColumns, limit, offset);
}

template <class T, class S>
inline QList<T> tfGetModelListByCriteria(const TCriteria &cri, QString sortColumn, Tf::SortOrder order, int limit = 0, int offset = 0)
{
    QList<QPair<QString, Tf::SortOrder>> sortColumns = {qMakePair(sortColumn, order)};
    return tfGetModelListByCriteria<T, S>(cri, sortColumns, limit, offset);
}

template <class T, class S>
inline QList<T> tfGetModelListByCriteria(const TCriteria &cri = TCriteria(), int limit = 0, int offset = 0)
{
    QList<QPair<int, Tf::SortOrder>> sortColumns = {qMakePair(-1, Tf::AscendingOrder)};
    return tfGetModelListByCriteria<T, S>(cri, sortColumns, limit, offset);
}


template <class T, class S>
inline QList<T> tfGetModelListByMongoCriteria(const TCriteria &cri, int sortColumn, Tf::SortOrder order, int limit = 0, int offset = 0)
{
    TMongoODMapper<S> mapper;

    if (sortColumn >= 0) {
        mapper.setSortOrder(sortColumn, order);
    }
    if (limit > 0) {
        mapper.setLimit(limit);
    }
    if (offset > 0) {
        mapper.setOffset(offset);
    }
    QList<T> list;
    if (mapper.find(cri)) {
        while (mapper.next()) {
            list << T(mapper.value());
        }
    }
    return list;
}


template <class T, class S>
inline QList<T> tfGetModelListByMongoCriteria(const TCriteria &cri, int limit = 0, int offset = 0)
{
    return tfGetModelListByMongoCriteria<T, S>(cri, -1, Tf::AscendingOrder, limit, offset);
}


template <class T>
inline QJsonArray tfConvertToJsonArray(const QList<T> &list, const QStringList &properties = QStringList())
{
    QJsonArray array;
    for (auto &it : list) {
        array.append(it.toJsonObject(properties));
    }
    return array;
}


#if QT_VERSION >= 0x050c00  // 5.12.0
template <class T>
inline QCborArray tfConvertToCborArray(const QList<T> &list)
{
    QCborArray array;
    for (auto &it : list) {
        array.append(QCborValue(it.toCborMap()));
    }
    return array;
}
#endif

