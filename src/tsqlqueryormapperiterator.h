#pragma once
#include <TSqlQueryORMapper>


template <class T>
class TSqlQueryORMapperIterator {
public:
    TSqlQueryORMapperIterator(TSqlQueryORMapper<T> &mapper) :
        m(&mapper), it(0) { }

    bool hasNext() const { return (it >= 0 && it < m->size()); }
    bool hasPrevious() const { return (it > 0 && it <= m->size()); }
    T next();
    T previous();
    void toBack()
    {
        m->last();
        it = m->size();
    }
    void toFront()
    {
        m->first();
        it = 0;
    }
    T value() const { return m->value(); }

private:
    TSqlQueryORMapperIterator(const TSqlQueryORMapperIterator<T> &);
    TSqlQueryORMapperIterator<T> &operator=(const TSqlQueryORMapperIterator<T> &);

    TSqlQueryORMapper<T> *m;
    int it;
};


/*!
  Returns the next object and advances the iterator by one position.
*/
template <class T>
inline T TSqlQueryORMapperIterator<T>::next()
{
    if (it++ != m->at()) {
        m->next();
    }
    return m->value();
}

/*!
  Returns the previous object and moves the iterator back by one
  position.
*/
template <class T>
inline T TSqlQueryORMapperIterator<T>::previous()
{
    if (--it != m->at()) {
        m->previous();
    }
    return m->value();
}

