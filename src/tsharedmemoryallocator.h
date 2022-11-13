#include "tmalloc.h"
#include "tshm.h"
#include <QString>
#include <memory>




template <typename T, const QString &path>
class TSharedMemoryAllocator {
public:
    using value_type = T;

    //template <typename U>
    //TSharedMemoryAllocator(const TSharedMemoryAllocator<U, path> &) {}

    // Constructor
    explicit TSharedMemoryAllocator() : _path(path)
    {
        //_ptr = Tf::create_shm(qPrintable(path), size, T());
    }

    T *allocate(std::size_t n)
    {
        return reinterpret_cast<T*>(Tf::tmalloc(sizeof(T) * n));
    }

    void deallocate(T *p, std::size_t n)
    {
        static_cast<void>(n);
        Tf::tfree(p);
    }

    template <class U> struct rebind { using other = TSharedMemoryAllocator<U, path>; };

    template <typename U>
    bool operator==(TSharedMemoryAllocator<U, path> const &other) const
    {
        return typeid(T) == typeid(U) && _path == other._path;
    }

    template <typename U>
    bool operator!=(TSharedMemoryAllocator<U, path> const &other) const
    {
        return typeid(T) != typeid(U) || _path != other._path;
    }

private:
    QString _path;
};


namespace Tf {
//void initializeSharedMemory(const QString &p, size_t size);

template <typename Container>
Container *createContainer(const QString &p, size_t maxSize)
{
    Tf::shmcreate(qPrintable(p), maxSize);
    return nullptr;
}

}
