#include "thazardpointermanager.h"
#include "thazardpointer.h"
#include "thazardobject.h"
#include <QThread>


THazardPointerManager hazardPointerManager;


class THazardRemoverThread : public QThread
{
public:
    THazardRemoverThread() : QThread() {  }
protected:
    void run();
};


void THazardRemoverThread::run()
{
    auto &hpm = hazardPointerManager;
    printf("I'm in.  obj-cnt:%d  hzp cnt:%d\n", hpm.objCount.load(), hpm.hprCount.load());

    for (;;) {
        int startObjCnt = hpm.objCount.load();
        int startHprCnt = hpm.hprCount.load();

        THazardObject *prevObj = nullptr;
        THazardObject *crtObj = hpm.objHead.load();
        THazardPointerRecord *hprhead = hpm.hprHead.load();

        while (crtObj) {
            THazardPointerRecord *hpr = hprhead;
            THazardPointerRecord *prevHpr = nullptr;
            const void *guardp = nullptr;

            while (hpr) {
                guardp = hpr->hazptr.load();
                if (guardp == (void*)0x01) {  // unused pointer
                    if (hpm.pop(hpr, prevHpr)) {
                        delete hpr;
                        hpr = prevHpr->next;
                        continue;
                    }
                }
                if (crtObj == guardp) {
                    break;
                }
                prevHpr = hpr;
                hpr = hpr->next;
            }

            if (crtObj != guardp && hpm.pop(crtObj, prevObj)) {
                delete crtObj;
                crtObj = prevObj->next;
            } else {
                prevObj = crtObj;
                crtObj = crtObj->next;
            }
        }

        int crtObjCnt = hpm.objCount.load();
        int crtHprCnt = hpm.hprCount.load();
        if (crtObjCnt < startObjCnt || crtObjCnt < 10 || crtHprCnt < startHprCnt) {
            break;
        }
    }
    printf("I'm out  obj-cnt:%d  hzp cnt:%d\n", hpm.objCount.load(), hpm.hprCount.load());
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
        ptr->next = hprHead.load();
    } while (!hprHead.compareExchange(ptr->next, ptr));
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
        obj->next = objHead.load();
    } while (!objHead.compareExchange(obj->next, obj));
    ++objCount;

    // Limits objects
    for (;;) {
        int objcnt = objCount.load();
        int hzcnt = hprCount.load();
        if (objcnt < (hzcnt + 1) * 4) {
            break;
        }
        gc();
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
        removerThread->start(QThread::TimeCriticalPriority);
    }
}
