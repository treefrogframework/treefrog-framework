#ifndef THAZARDPOINTER_H
#define THAZARDPOINTER_H

#include <TGlobal>
#include "tatomicptr.h"

class THazardObject;
class THazardPointerRecord;


class T_CORE_EXPORT THazardPointer
{
public:
    THazardPointer();
    ~THazardPointer();

    template <typename T> T *guard(TAtomicPtr<T> *src);
    void guard(THazardObject *ptr);
    void clear();

private:
    THazardPointerRecord *rec;

    enum { Mask = 0x3 };
    friend class THazardPointerManager;
};


class T_CORE_EXPORT THazardPointerRecord
{
public:
    THazardPointerRecord() { }
    ~THazardPointerRecord() { }

    TAtomicPtr<THazardObject> hazptr { nullptr };
    THazardPointerRecord *next { nullptr };
};


template <typename T>
inline T *THazardPointer::guard(TAtomicPtr<T>  *src)
{
    T *ptr = src->load();
    rec->hazptr.store((THazardObject*)((quintptr)ptr & ~Mask));  // 4byte alignment
    return ptr;
}

#endif // THAZARDPOINTER_H
