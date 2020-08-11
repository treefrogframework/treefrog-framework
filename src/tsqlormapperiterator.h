#pragma once
#include <TSqlORMapper>


template <class T>
class TSqlORMapperIterator {
public:
    TSqlORMapperIterator(const TSqlORMapper<T> &mapper) :
        m(&mapper), i(0), n(m->rowCount()) { }

    bool hasNext() const { return i >= 0 && i < m->rowCount(); }
    bool hasPrevious() const { return i > 0 && i <= m->rowCount(); }
    T next()
    {
        n = i++;
        return m->value(n);
    }
    T previous()
    {
        n = --i;
        return m->value(n);
    }
    void toBack() { i = n = m->rowCount(); }
    void toFront()
    {
        i = 0;
        n = m->rowCount();
    }
    T value() const { return m->value(n); }

private:
    TSqlORMapperIterator(const TSqlORMapperIterator<T> &);
    TSqlORMapperIterator<T> &operator=(const TSqlORMapperIterator<T> &);

    const TSqlORMapper<T> *m;
    int i, n;
};

