#ifndef TATOMICPTR_H
#define TATOMICPTR_H

#include <atomic>

namespace Tf
{
    inline void threadFence()
    {
        atomic_thread_fence(std::memory_order_seq_cst);
    }
}


template <class T>
class TAtomicPtr
{
public:
    TAtomicPtr(T *value = nullptr);
    TAtomicPtr(const TAtomicPtr<T> &other);
    ~TAtomicPtr() {}

    operator T*() const;
    T *load() const;
    void store(T *value);
    bool compareExchange(T *expected, T *newValue);
    TAtomicPtr<T> &operator=(T *value);
    TAtomicPtr<T> &operator=(const TAtomicPtr<T> &other);

    void mark();
    void unmark();
    bool isMarked() const;

private:
    std::atomic<T*> atomicPtr { nullptr };

    // Deleted functions
    TAtomicPtr(TAtomicPtr &&) = delete;
    TAtomicPtr &operator=(TAtomicPtr &&) = delete;
};


template <class T>
inline TAtomicPtr<T>::TAtomicPtr(T *value)
    : atomicPtr(value)
{ }


template <class T>
inline TAtomicPtr<T>::TAtomicPtr(const TAtomicPtr<T> &other)
    : atomicPtr(other.atomicPtr.load())
{ }


template <class T>
inline TAtomicPtr<T>::operator T*() const
{
    return load();
}


template <class T>
inline T *TAtomicPtr<T>::load() const
{
    return atomicPtr.load(std::memory_order_acquire);
}


template <class T>
inline void TAtomicPtr<T>::store(T *value)
{
    atomicPtr.store(value, std::memory_order_release);
}


template <class T>
inline bool TAtomicPtr<T>::compareExchange(T *expected, T *newValue)
{
    return atomicPtr.compare_exchange_weak(expected, newValue);
}


template <class T>
inline TAtomicPtr<T> &TAtomicPtr<T>::operator=(T *value)
{
    store(value);
    return *this;
}


template <class T>
inline TAtomicPtr<T> &TAtomicPtr<T>::operator=(const TAtomicPtr<T> &other)
{
    store(other.load());
    return *this;
}


template <class T>
inline void TAtomicPtr<T>::mark()
{
    for (;;) {
        T* ptr = load();
        if ((ptr & 0x1) || compareExchange(ptr, ptr | 0x1)) {
            break;
        }
    }
}


template <class T>
inline void TAtomicPtr<T>::unmark()
{
    for (;;) {
        T* ptr = load();
        if (!(ptr & 0x1) || compareExchange(ptr, ptr & ~0x1)) {
            break;
        }
    }
}


template <class T>
inline bool TAtomicPtr<T>::isMarked() const
{
    return load() & 0x1;
}

#endif // TATOMICPTR_H
