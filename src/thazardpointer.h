#ifndef THAZARDPOINTER_H
#define THAZARDPOINTER_H

#include <TGlobal>
#include <atomic>

class THazardObject;
class THazardPointerRecord;


class T_CORE_EXPORT THazardPointer
{
public:
    THazardPointer();
    ~THazardPointer();

    template <typename T> T *guard(std::atomic<T*> *src);
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

    std::atomic<THazardObject*> hazptr { nullptr };
    THazardPointerRecord *next { nullptr };
};


template <typename T>
inline T *THazardPointer::guard(std::atomic<T*> *src)
{
    T *ptr = src->load(std::memory_order_acquire);
    rec->hazptr.store((THazardObject*)((quintptr)ptr & ~Mask), std::memory_order_release);  // 4byte alignment
    return ptr;
}

#endif // THAZARDPOINTER_H
