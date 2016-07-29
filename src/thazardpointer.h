#ifndef THAZARDPOINTER_H
#define THAZARDPOINTER_H

#include <atomic>
#include <TGlobal>

class THazardObject;
class THazardPointerRecord;


class T_CORE_EXPORT THazardPointer
{
public:
    THazardPointer();
    ~THazardPointer();

    template <typename T> bool guard(const T *ptr, const std::atomic<T*> *src);
    void set(const THazardObject *ptr);
    void clear();

    static void swap(THazardPointer &a, THazardPointer &b);

private:
    THazardPointerRecord *rec;

    friend class THazardPointerManager;
};


class T_CORE_EXPORT THazardPointerRecord
{
public:
    THazardPointerRecord() { }
    ~THazardPointerRecord() { }

    std::atomic<const THazardObject*> hazptr { nullptr };
    THazardPointerRecord *next { nullptr };
};


template <typename T>
inline bool THazardPointer::guard(const T *ptr, const std::atomic<T*> *src)
{
    rec->hazptr.store(ptr, std::memory_order_release);
    return (ptr == src->load(std::memory_order_acquire));
}

#endif // THAZARDPOINTER_H
