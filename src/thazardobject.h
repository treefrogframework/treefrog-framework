#ifndef THAZARDOBJECT_H
#define THAZARDOBJECT_H

#include <TGlobal>
#include <atomic>


class T_CORE_EXPORT THazardObject
{
public:
    THazardObject() : next(nullptr), deleted(false) {}
    THazardObject(const THazardObject &) : next(nullptr), deleted(false) {}
    ~THazardObject() {}

    void deleteLater();
    THazardObject &operator=(const THazardObject &) { return *this; }

private:
    THazardObject *next { nullptr };
    std::atomic<bool> deleted { false };

    friend class THazardPointerManager;

    // Deleted functions
    THazardObject(THazardObject &&) = delete;
    THazardObject &operator=(THazardObject &&) = delete;
};

#endif // THAZARDOBJECT_H
