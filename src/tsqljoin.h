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
    TSqlJoin(int joinColumn, const TCriteria &criteria);
    TSqlJoin(TSql::JoinMode joinMode, int joinColumn, const TCriteria &criteria);

    TSql::JoinMode joinMode() const { return _mode; }
    int joinColumn() const { return _joinColumn; }
    TCriteria criteria() const { return _criteria; }

private:
    TSql::JoinMode _mode {TSql::InnerJoin};
    int _joinColumn {-1};
    TCriteria _criteria;
};


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
