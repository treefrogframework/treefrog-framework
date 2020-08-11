#pragma once
#include "tatomic.h"
#include "tatomicptr.h"
#include <TGlobal>

class THazardPtrRecord;
class THazardObject;
class THazardRemoverThread;


class T_CORE_EXPORT THazardPtrManager {
public:
    ~THazardPtrManager();

    void setGarbageCollectionBufferSize(int size);
    static THazardPtrManager &instance();

private:
    void push(THazardPtrRecord *ptr);
    bool pop(THazardPtrRecord *ptr, THazardPtrRecord *prev);
    void push(THazardObject *obj);
    bool pop(THazardObject *ptr, THazardObject *prev);
    void gc();

    THazardPtrManager();  // constructor

    TAtomicPtr<THazardPtrRecord> hprHead {nullptr};
    TAtomic<int> hprCount {0};
    TAtomicPtr<THazardObject> objHead {nullptr};
    TAtomic<int> objCount {0};
    int gcBufferSize {100};
    THazardRemoverThread *removerThread {nullptr};

    friend class THazardPtr;
    friend class THazardObject;
    friend class THazardRemoverThread;

    T_DISABLE_COPY(THazardPtrManager)
    T_DISABLE_MOVE(THazardPtrManager)
};

