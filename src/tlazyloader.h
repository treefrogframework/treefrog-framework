#pragma once
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
    if (lazyObject()->isEmpty()) {
        TSqlORMapper<T> mapper;
        //*lazyObject() = mapper.findByPrimaryKey(pkey);

        abort();
        // ここは実装しなおし、QSharedDataPointer を参考に！
    }
}

