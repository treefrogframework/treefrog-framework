#ifndef THAZARDOBJECT_H
#define THAZARDOBJECT_H

#include <TGlobal>
#include <atomic>


class T_CORE_EXPORT THazardObject
{
public:
    THazardObject();
    THazardObject(const THazardObject &);
    THazardObject(THazardObject &&) {}
    virtual ~THazardObject() {}

    void deleteLater();
    THazardObject &operator=(const THazardObject &) { return *this; }
    THazardObject &operator=(THazardObject &&) { return *this; }

private:
    THazardObject *next { nullptr };
    std::atomic_flag deleted { ATOMIC_FLAG_INIT };

    friend class THazardPointerManager;
    friend class THazardRemoverThread;
};

#endif // THAZARDOBJECT_H
