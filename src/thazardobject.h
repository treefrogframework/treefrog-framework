#pragma once
#include <TAtomic>
#include <TGlobal>


class T_CORE_EXPORT THazardObject {
public:
    THazardObject();
    THazardObject(const THazardObject &);
    THazardObject(THazardObject &&) { }
    virtual ~THazardObject() { }

    void deleteLater();
    THazardObject &operator=(const THazardObject &) { return *this; }
    THazardObject &operator=(THazardObject &&) { return *this; }

private:
    THazardObject *next {nullptr};
    TAtomic<bool> deleted {false};

    friend class THazardPtrManager;
    friend class THazardRemoverThread;
};

