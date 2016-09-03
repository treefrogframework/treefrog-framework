#ifndef THAZARDPTR_H
#define THAZARDPTR_H

#include <TGlobal>
#include "tatomicptr.h"

class THazardObject;
class THazardPtrRecord;


class T_CORE_EXPORT THazardPtr
{
public:
    THazardPtr();
    ~THazardPtr();

    template <typename T> T *guard(TAtomicPtr<T> *src);
    void guard(THazardObject *ptr);
    void clear();

private:
    THazardPtrRecord *rec { nullptr };

    enum { Mask = 0x3 };
    friend class THazardPtrManager;
};


class T_CORE_EXPORT THazardPtrRecord
{
public:
    THazardPtrRecord() { }
    ~THazardPtrRecord() { }

    TAtomicPtr<THazardObject> hazptr { nullptr };
    THazardPtrRecord *next { nullptr };
};


template <typename T>
inline T *THazardPtr::guard(TAtomicPtr<T>  *src)
{
    T *ptr = src->load();
    rec->hazptr.store((THazardObject*)((quintptr)ptr & ~Mask));  // 4byte alignment
    return ptr;
}

#endif // THAZARDPTR_H
