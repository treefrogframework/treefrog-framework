#ifndef TSQLQUERYORMAPPERITERATOR_H
#define TSQLQUERYORMAPPERITERATOR_H

#include <TSqlQueryORMapper>


template <class T>
class TSqlQueryORMapperIterator
{
public:
    TSqlQueryORMapperIterator(TSqlQueryORMapper<T> &mapper) : m(&mapper), it(0) { }

    bool hasNext() const { return (it >= 0 && it < m->size()); }
    bool hasPrevious() const { return (it > 0 && it <= m->size()); }
    T next();
    T previous();
    void toBack() { m->last(); it = m->size(); }
    void toFront() { m->first(); it = 0; }
    T value() const { return m->value(); }
        
private:
    TSqlQueryORMapperIterator(const TSqlQueryORMapperIterator<T> &);
    TSqlQueryORMapperIterator<T> &operator=(const TSqlQueryORMapperIterator<T> &);

    TSqlQueryORMapper<T> *m;
    int it;
};


template <class T>
inline T TSqlQueryORMapperIterator<T>::next()
{
    if (it++ != m->at()) {
        m->next();
    }
    return m->value();
}


template <class T>
inline T TSqlQueryORMapperIterator<T>::previous()
{
    if (--it != m->at()) {
        m->previous();
    }
    return m->value();
}

#endif // TSQLQUERYORMAPPERITERATOR_H
