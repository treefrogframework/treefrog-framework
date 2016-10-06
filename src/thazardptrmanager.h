#ifndef THAZARDPTRMANAGER_H
#define THAZARDPTRMANAGER_H

#include <TGlobal>
#include <atomic>
#include "tatomicptr.h"

class THazardPtrRecord;
class THazardObject;
class THazardRemoverThread;


class T_CORE_EXPORT THazardPtrManager
{
public:
    THazardPtrManager();
    ~THazardPtrManager();

private:
    void push(THazardPtrRecord *ptr);
    bool pop(THazardPtrRecord *ptr, THazardPtrRecord *prev);
    void push(THazardObject *obj);
    bool pop(THazardObject *ptr, THazardObject *prev);
    void gc();
    static THazardPtrManager &instance();

    TAtomicPtr<THazardPtrRecord> hprHead {nullptr};
    std::atomic<int> hprCount {0};
    TAtomicPtr<THazardObject> objHead {nullptr};
    std::atomic<int> objCount {0};
    THazardRemoverThread *removerThread {nullptr};

    friend class THazardPtr;
    friend class THazardObject;
    friend class THazardRemoverThread;

    // Deleted functions
    THazardPtrManager(const THazardPtrManager &) = delete;
    THazardPtrManager(THazardPtrManager &&) = delete;
    THazardPtrManager &operator=(const THazardPtrManager &) = delete;
    THazardPtrManager &operator=(THazardPtrManager &&) = delete;
};

#endif // THAZARDPTRMANAGER_H
