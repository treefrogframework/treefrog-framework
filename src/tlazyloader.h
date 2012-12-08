#ifndef TLAZYLOADER_H
#define TLAZYLOADER_H

#include <TGlobal>


template <typename T, typename Key>
class TLazyLoader {
protected:
    void setLazyLoadKey(const Key &pk) { pkey = pk; }
    void loadObject();
    virtual T *lazyObject() = 0;

private:
    Key pkey;
};


template <typename T, typename Key>
inline void TLazyLoader<T, Key>::loadObject()
{
    T_TRACEFUNC();
    if (lazyObject()->isEmpty()) {
        TSqlORMapper<T> mapper;
        //*lazyObject() = mapper.findByPrimaryKey(pkey);

        abort();
        // ここは実装しなおし、QSharedDataPointer を参考に！
    }
}

#endif // TLAZYLOADER_H
