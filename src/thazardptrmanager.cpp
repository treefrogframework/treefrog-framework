#include "thazardptrmanager.h"
#include "thazardobject.h"
#include "thazardptr.h"
#include <QThread>


class THazardRemoverThread : public QThread {
public:
    THazardRemoverThread() :
        QThread() { }

protected:
    void run();
};


void THazardRemoverThread::run()
{
    auto &hpm = THazardPtrManager::instance();
    //printf("I'm in.  obj-cnt:%d  hzp cnt:%d\n", hpm.objCount.load(), hpm.hprCount.load());

    for (;;) {
        int startObjCnt = hpm.objCount;
        int startHprCnt = hpm.hprCount;

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
                    if (Q_LIKELY(hpm.pop(hpr, prevHpr))) {
                        delete hpr;
                        hpr = prevHpr->next;
                        continue;
                    }
                }
                if (Q_UNLIKELY(crtObj == guardp)) {
                    break;
                }
                prevHpr = hpr;
                hpr = hpr->next;  // to next
            }

            if (crtObj != guardp && hpm.pop(crtObj, prevObj)) {
                delete crtObj;
                crtObj = prevObj->next;
            } else {
                prevObj = crtObj;
                crtObj = crtObj->next;
            }
        }

        if ((int)hpm.objCount <= startObjCnt || (int)hpm.hprCount <= startHprCnt) {
            break;
        }
    }
    //printf("I'm out  obj-cnt:%d  hzp cnt:%d\n", (int)hpm.objCount, (int)hpm.hprCount);
}


THazardPtrManager::THazardPtrManager() :
    removerThread(new THazardRemoverThread())
{
}


THazardPtrManager::~THazardPtrManager()
{
    removerThread->wait();
    delete removerThread;
}


void THazardPtrManager::setGarbageCollectionBufferSize(int size)
{
    gcBufferSize = qMax(size, 100);
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
    if (Q_LIKELY(ptr && prev)) {
        prev->next = ptr->next;
        hprCount--;
        return true;
    }
    return false;
}


void THazardPtrManager::push(THazardObject *obj)
{
    do {
        obj->next = objHead.load();
    } while (!objHead.compareExchange(obj->next, obj));
    objCount++;

    // Limits objects
    for (;;) {
        int limit = qMax(gcBufferSize, (int)hprCount * 2);
        if ((int)objCount < limit) {
            break;
        }
        gc();
    }
}


bool THazardPtrManager::pop(THazardObject *obj, THazardObject *prev)
{
    if (Q_LIKELY(obj && prev)) {
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


THazardPtrManager &THazardPtrManager::instance()
{
    static THazardPtrManager hazardPtrManager;
    return hazardPtrManager;
}
