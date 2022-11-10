#include "tmalloc.h"


namespace Tf {
void initializeSharedMemory(size_t size);
//void syncSharedMemory();
}


template <class T>
class TSharedMemoryAllocator {
public:
    using value_type = T;
    //template <typename U> friend class TSharedMemoryAllocator;

    // Constructor
    explicit TSharedMemoryAllocator() { Tf::initializeSharedMemory(256 * 1024 * 1024); }

    template <class U>
    TSharedMemoryAllocator(const TSharedMemoryAllocator<U> &) {}

    T *allocate(std::size_t n)
    {
        return reinterpret_cast<T*>(Tf::tmalloc(sizeof(T) * n));
    }

    void deallocate(T *p, std::size_t n)
    {
        static_cast<void>(n);
        Tf::tfree(p);
    }

    // template <typename U>
    // bool operator==(TSharedMemoryAllocator<U> const &) const
    // {
    //     return true;
    // }

    // template <typename U>
    // bool operator!=(TSharedMemoryAllocator<U> const &) const
    // {
    //     return false;
    // }
};
