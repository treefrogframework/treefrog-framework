#include "thazardptrmanager.h"
#include "thazardptr.h"
#include "thazardobject.h"
#include <QThread>


THazardPtrManager hazardPtrManager;


class THazardRemoverThread : public QThread
{
public:
    THazardRemoverThread() : QThread() {  }
protected:
    void run();
};


void THazardRemoverThread::run()
{
    auto &hpm = hazardPtrManager;
    //printf("I'm in.  obj-cnt:%d  hzp cnt:%d\n", hpm.objCount.load(), hpm.hprCount.load());

    for (;;) {
        int startObjCnt = hpm.objCount.load();
        int startHprCnt = hpm.hprCount.load();

        THazardObject *prevObj = nullptr;
        THazardObject *crtObj = hpm.objHead.load();
        THazardPtrRecord *hprhead = hpm.hprHead.load();

        while (crtObj) {
            THazardPtrRecord *hpr = hprhead;
            THazardPtrRecord *prevHpr = nullptr;
            const void *guardp = nullptr;
            bool mark;

            while (hpr) {
                guardp = hpr->hazptr.load(&mark);
                if (mark && guardp == nullptr) {  // unused pointer
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
        if (crtObjCnt < startObjCnt || crtHprCnt < startHprCnt) {
            break;
        }
    }
    //printf("I'm out  obj-cnt:%d  hzp cnt:%d\n", hpm.objCount.load(), hpm.hprCount.load());
}


THazardPtrManager::THazardPtrManager()
    : removerThread(new THazardRemoverThread())
{ }


THazardPtrManager::~THazardPtrManager()
{
    removerThread->wait();
    delete removerThread;
}


void THazardPtrManager::push(THazardPtrRecord *ptr)
{
    do {
        ptr->next = hprHead.load();
    } while (!hprHead.compareExchange(ptr->next, ptr));
    hprCount++;
}


bool THazardPtrManager::pop(THazardPtrRecord *ptr, THazardPtrRecord *prev)
{
    if (ptr && prev) {
        prev->next = ptr->next;
        hprCount--;
        return true;
    }
    return false;
}


void THazardPtrManager::push(THazardObject* obj)
{
    do {
        obj->next = objHead.load();
    } while (!objHead.compareExchange(obj->next, obj));
    objCount++;

    // Limits objects
    for (;;) {
        int hzcnt = hprCount.load();
        int objcnt = objCount.load();
        if (objcnt < (hzcnt + 1) * 2) {
            break;
        }
        gc();
    }
}


bool THazardPtrManager::pop(THazardObject *obj, THazardObject *prev)
{
    if (obj && prev) {
        prev->next = obj->next;
        objCount--;
        return true;
    }
    return false;
}


void THazardPtrManager::gc()
{
    if (!removerThread->isRunning()) {
        removerThread->start(QThread::HighestPriority);
    }
}
