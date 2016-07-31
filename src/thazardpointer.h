#ifndef THAZARDPOINTER_H
#define THAZARDPOINTER_H

#include <atomic>
#include <TGlobal>
//#include <QAtomicPointer>

class THazardObject;
class THazardPointerRecord;


class T_CORE_EXPORT THazardPointer
{
public:
    THazardPointer();
    ~THazardPointer();

    //template <typename T> bool guard(const T *ptr, const std::atomic<T*> *src);
    template <typename T> T *guard(std::atomic<T*> *src);
    template <typename T> T *guard(QAtomicPointer<T> *src);
    void set(THazardObject *ptr);
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

#if 1
    std::atomic<const THazardObject*> hazptr { nullptr };
#else
    QAtomicPointer<THazardObject> hazptr { nullptr };
#endif
    THazardPointerRecord *next { nullptr };
};


// template <typename T>
// inline bool THazardPointer::guard(const T *ptr, const std::atomic<T*> *src)
// {
//     rec->hazptr.store(ptr, std::memory_order_release);
//     return (ptr == src->load(std::memory_order_acquire));
// }

#if 1
template <typename T>
inline T *THazardPointer::guard(std::atomic<T*> *src)
{
//    std::atomic_thread_fence(std::memory_order_seq_cst);

    T *ptr = src->load();
//    std::atomic_thread_fence(std::memory_order_seq_cst);

    rec->hazptr.store(ptr);
//    std::atomic_thread_fence(std::memory_order_seq_cst);
    return ptr;
}
#else
template <typename T>
inline T *THazardPointer::guard(QAtomicPointer<T> *src)
{
    T *ptr = (T*)(*src);
    rec->hazptr.fetchAndStoreRelease(ptr);
    return ptr;
}
#endif

#endif // THAZARDPOINTER_H
