#pragma once
#include <TAtomic>
#include "tdeclexport.h"


class T_CORE_EXPORT THazardObject {
public:
    THazardObject();
    THazardObject(const THazardObject &);
    THazardObject &operator=(const THazardObject &) { return *this; }
    THazardObject(THazardObject &&) = delete;
    THazardObject &operator=(THazardObject &&) = delete;
    virtual ~THazardObject() { }

    void deleteLater();

private:
    THazardObject *next {nullptr};
    TAtomic<bool> deleted {false};

    friend class THazardPtrManager;
    friend class THazardRemoverThread;
};
