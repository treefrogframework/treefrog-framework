#pragma once
#include <TCriteria>
#include <TGlobal>

/*!
  \class TSqlJoin
  \brief The TSqlJoin class represents JOIN clause for combination
  to a record of other table.
  \see void TSqlORMapper<T>::setJoin()
*/


template <class T>
class TSqlJoin {
public:
    TSqlJoin();
    TSqlJoin(int joinColumn, const TCriteria &criteria);
    TSqlJoin(TSql::JoinMode joinMode, int joinColumn, const TCriteria &criteria);
    TSqlJoin(const TSqlJoin &other);

    TSqlJoin &operator=(const TSqlJoin &other);
    TSql::JoinMode joinMode() const { return _mode; }
    int joinColumn() const { return _joinColumn; }
    TCriteria criteria() const { return _criteria; }

private:
    TSql::JoinMode _mode;
    int _joinColumn;
    TCriteria _criteria;
};


template <class T>
inline TSqlJoin<T>::TSqlJoin() :
    _mode(TSql::InnerJoin), _joinColumn(-1), _criteria()
{
}

template <class T>
inline TSqlJoin<T>::TSqlJoin(int joinColumn, const TCriteria &criteria) :
    _mode(TSql::InnerJoin), _joinColumn(joinColumn), _criteria(criteria)
{
}

template <class T>
inline TSqlJoin<T>::TSqlJoin(TSql::JoinMode joinMode, int joinColumn, const TCriteria &criteria) :
    _mode(joinMode), _joinColumn(joinColumn), _criteria(criteria)
{
}

template <class T>
inline TSqlJoin<T>::TSqlJoin(const TSqlJoin &other) :
    _mode(other.mode), _joinColumn(other._joinColumn), _criteria(other._criteria)
{
}

template <class T>
inline TSqlJoin<T> &TSqlJoin<T>::operator=(const TSqlJoin &other)
{
    _mode = other._mode;
    _joinColumn = other._joinColumn;
    _criteria = other._criteria;
}

