#pragma once
#include "tatomicptr.h"
#include <TGlobal>

class THazardObject;
class THazardPtrRecord;


class T_CORE_EXPORT THazardPtr {
public:
    THazardPtr();
    ~THazardPtr();

    template <typename T>
    T *guard(TAtomicPtr<T> *src, bool *mark = nullptr);
    void guard(THazardObject *ptr);
    void clear();

private:
    THazardPtrRecord *rec {nullptr};

    friend class THazardPtrManager;
};


class T_CORE_EXPORT THazardPtrRecord {
public:
    THazardPtrRecord() { }
    ~THazardPtrRecord() { }

    TAtomicPtr<THazardObject> hazptr {nullptr};
    THazardPtrRecord *next {nullptr};
};


template <typename T>
inline T *THazardPtr::guard(TAtomicPtr<T> *src, bool *mark)
{
    T *ptr = src->load(mark);
    rec->hazptr.store(ptr);
    return ptr;
}

