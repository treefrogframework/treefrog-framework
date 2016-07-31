#include "thazardpointermanager.h"
#include "thazardpointer.h"
#include "thazardobject.h"
#include <QThread>


THazardPointerManager hazardPointerManager;


class THazardRemoverThread : public QThread
{
public:
    THazardRemoverThread() : QThread() { setPriority(QThread::TimeCriticalPriority); }
protected:
    void run();
};


void THazardRemoverThread::run()
{
    auto &hpm = hazardPointerManager;
    int loopcnt = 0;
printf("I'm in.... (%d)\n", loopcnt);
    int startObjCnt = hpm.objCount.load(std::memory_order_acquire);
    //int startHprCnt = hpm.hprCount.load(std::memory_order_acquire);

    for(;;) {

    // printf("I'm in.... (%d)\n", loopcnt);
    // if (loopcnt > 10) {
    // }
printf("I'm in.  obj-cnt:%d  hzp cnt:%d\n", hpm.objCount.load(std::memory_order_acquire), hpm.hprCount.load(std::memory_order_acquire));

    THazardObject *prevObj = nullptr;
    THazardObject *crtObj = hpm.objHead.load(std::memory_order_acquire);
    THazardPointerRecord *hprhead = hpm.hprHead.load(std::memory_order_acquire);

    while (crtObj) {
        THazardPointerRecord *hpr = hprhead;
        THazardPointerRecord *prevHpr = nullptr;
        const void *guardp = nullptr;

        while (hpr) {
#if 0
            guardp = hpr->hazptr.load(std::memory_order_acquire);
#else
            guardp = hpr->hazptr;
#endif

            if (guardp == (void*)0x01) {  // unused pointer
                if (hpm.pop(hpr, prevHpr)) {
//printf("delete hzp  %p\n", hzp);
                    delete hpr;
                    hpr = prevHpr->next;
                    continue;
                }
            }
            if (crtObj == guardp) {
//printf("guard object %p  hzp cnt:%d  obj cnt:%d\n", obj, hprCount.load(), objCount.load());
                break;
            }
            prevHpr = hpr;
            hpr = hpr->next;
        }

        if (crtObj != guardp && hpm.pop(crtObj, prevObj)) {
            // static int cnt = 0;
            // printf("deleted obj cnt=%d  remain:%d\n", ++cnt, hpm.objCount.load());

            delete crtObj;
            crtObj = prevObj->next;
        } else {
            prevObj = crtObj;
            crtObj = crtObj->next;
        }
        //       printf("################# next:%p  obj-cnt:%d  hzp cnt:%d\n", obj, objCount.load(), hprCount.load());
    }

    int cntCnt = hpm.objCount.load(std::memory_order_acquire);
    if (cntCnt <= startObjCnt || cntCnt < 10) {
        break;
    }
    //Tf::msleep(1);
    }
    // unlock
    //hpm.gcFlag.clear(std::memory_order_release);
    printf("I'm out  obj-cnt:%d  hzp cnt:%d\n", hpm.objCount.load(std::memory_order_acquire), hpm.hprCount.load(std::memory_order_acquire));
}


THazardPointerManager::THazardPointerManager()
    : removerThread(new THazardRemoverThread())
{ }


THazardPointerManager::~THazardPointerManager()
{
    removerThread->wait();
    delete removerThread;
}


void THazardPointerManager::push(THazardPointerRecord *ptr)
{
    do {
        ptr->next = hprHead.load(std::memory_order_acquire);
    } while (!hprHead.compare_exchange_weak(ptr->next, ptr));
    ++hprCount;
}


bool THazardPointerManager::pop(THazardPointerRecord *ptr, THazardPointerRecord *prev)
{
    if (ptr && prev) {
        prev->next = ptr->next;
        --hprCount;
        return true;
    }
    return false;
}


void THazardPointerManager::push(THazardObject* obj)
{
    do {
        obj->next = objHead.load(std::memory_order_acquire);
    } while (!objHead.compare_exchange_weak(obj->next, obj));
    ++objCount;

    // Limits objects
    for (;;) {
        int objcnt = objCount.load(std::memory_order_acquire);
        int hzcnt = hprCount.load(std::memory_order_acquire);
        if (objcnt < hzcnt * 5) {
            break;
        }
        QThread::yieldCurrentThread();
    }
}


bool THazardPointerManager::pop(THazardObject *obj, THazardObject *prev)
{
    if (obj && prev) {
        prev->next = obj->next;
        --objCount;
        return true;
    }
    return false;
}


void THazardPointerManager::gc()
{
    if (!removerThread->isRunning()) {
        removerThread->start();
    }
}


// void THazardPointerManager::gcHazardPointers()
// {
//     if (gcFlag.test_and_set(std::memory_order_acquire)) {
//         return;
//     }

//     THazardPointerRecord *prevHpr = nullptr;
//     THazardPointerRecord *hpr = hprHead.load(std::memory_order_acquire);
//     while (hpr) {
//         if (hpr->hazptr.load(std::memory_order_acquire) == (void*)0x01) {  // unused pointer
//             if (pop(hpr, prevHpr)) {
//                 delete hpr;
//                 hpr = prevHpr->next;
//                 continue;
//             }
//         }
//         prevHpr = hpr;
//         hpr = hpr->next;
// //printf("gcHazardPointers prev:%p  cur:%p\n", prevHpr, hpr);
//     }

//     // unlock
//     gcFlag.clear();
// }
