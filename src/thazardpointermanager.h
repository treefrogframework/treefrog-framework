#ifndef THAZARDPOINTERMANAGER_H
#define THAZARDPOINTERMANAGER_H

#include <atomic>
#include <TGlobal>

class THazardPointerRecord;
class THazardObject;


class T_CORE_EXPORT THazardPointerManager
{
public:
    THazardPointerManager();
    ~THazardPointerManager();

private:
    void push(THazardPointerRecord *ptr);
    bool pop(THazardPointerRecord *ptr, THazardPointerRecord *prev);
    void push(THazardObject *obj);
    bool pop(THazardObject *ptr, THazardObject *prev);
    void gc();

    std::atomic<THazardPointerRecord*> hprHead;
    std::atomic<int> hprCount { 0 };
    std::atomic<THazardObject*> objHead;
    std::atomic<int> objCount { 0 };
    std::atomic<bool> gcFlag { false };

    friend class THazardPointer;
    friend class THazardObject;

    // Deleted functions
    THazardPointerManager(const THazardPointerManager &) = delete;
    THazardPointerManager(THazardPointerManager &&) = delete;
    THazardPointerManager &operator=(const THazardPointerManager &) = delete;
    THazardPointerManager &operator=(THazardPointerManager &&) = delete;
};

#endif // THAZARDPOINTERMANAGER_H
