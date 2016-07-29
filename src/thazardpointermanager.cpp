#include "thazardpointermanager.h"
#include "thazardpointer.h"
#include "thazardobject.h"

THazardPointerManager hazardPointerManager;


THazardPointerManager::THazardPointerManager()
{ }


THazardPointerManager::~THazardPointerManager()
{ }


void THazardPointerManager::push(THazardPointerRecord *ptr)
{
//printf("THazardPointerManager::push  %p\n", ptr);
    do {
        ptr->next = hprHead.load(std::memory_order_acquire);
//printf("THazardPointerManager::push load  %p\n", ptr->next );
        Q_ASSERT(ptr->next != ptr);
    } while (!hprHead.compare_exchange_weak(ptr->next, ptr));
    ++hprCount;
}


bool THazardPointerManager::pop(THazardPointerRecord *ptr, THazardPointerRecord *prev)
{
    if (ptr && prev) {
        prev->next = ptr->next;
//printf("pop THazardPointerRecord  ptr=%p  prev:%p  next:%p\n", ptr, prev, prev->next);
        if (prev->next == prev) {
//printf("pop THazardPointerRecord !!!  ptr=%p  prev:%p  next:%p\n", ptr, prev, prev->next);
            Q_ASSERT(0);
        }
        --hprCount;
        return true;
    }
    return false;
}


void THazardPointerManager::push(THazardObject* obj)
{
printf("push THazardObject %p\n", obj);
    do {
        obj->next = objHead.load(std::memory_order_acquire);
//   printf("THazardPointerManager::push  x=%d\n", x);
        Q_ASSERT(obj->next != obj);
    } while (!objHead.compare_exchange_weak(obj->next, obj));
    ++objCount;
}


bool THazardPointerManager::pop(THazardObject *obj, THazardObject *prev)
{
    if (obj && prev) {
        Q_ASSERT(prev->next == obj);
        prev->next = obj->next;
        Q_ASSERT(prev->next != prev);
        --objCount;
printf("obj pop : %d\n", objCount.load());
        return true;
    }
    return false;
}


void THazardPointerManager::gc()
{
    if (hprCount.load() <= 3 && objCount.load() <= 3) {
        return;
    }

//    printf("load:%d hzp:%d obj:%d\n", gcFlag.load(), hzpCount.load(), objCount.load());
    // try lock
    bool False = 0;
    if (!gcFlag.compare_exchange_strong(False, true)) {
        return;
    }
printf("I'm in....\n");

    THazardObject *prevObj = nullptr;
    THazardObject *obj = objHead.load();
printf("head obj  %p\n", obj);

    while (obj) {
        THazardPointerRecord *hpr = hprHead.load();
        THazardPointerRecord *prevHpr = nullptr;
        const void *guardp = nullptr;

        while (hpr) {
            guardp = hpr->hazptr.load();

            if (guardp == (void*)0x01) {  // unused pointer
                if (pop(hpr, prevHpr)) {
//printf("delete hzp  %p\n", hzp);
                    delete hpr;
                    hpr = prevHpr->next;
                    continue;
                }
            }
            if (obj == guardp) {
                break;
            }
            prevHpr = hpr;
            hpr = hpr->next;

//            printf("I'm in....  %p : cnt:%d\n", hpr, hprCount.load());
        }

        if (obj != guardp && pop(obj, prevObj)) {
   printf("delete obj  %p\n", obj);
            delete obj;
            obj = prevObj->next;
        } else {
            prevObj = obj;
            obj = obj->next;
        }
    }

    // unlock
    gcFlag.store(0, std::memory_order_release);

    printf("################# hzp:%d obj:%d\n", hprCount.load(), objCount.load());
}
