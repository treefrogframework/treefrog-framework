#include "thazardobject.h"
#include "thazardptrmanager.h"


THazardObject::THazardObject()
{
    THazardPtrManager::instance().gc();
}


THazardObject::THazardObject(const THazardObject &)
{
    THazardPtrManager::instance().gc();
}


void THazardObject::deleteLater()
{
    if (!deleted.test_and_set(std::memory_order_acquire)) {
        THazardPtrManager::instance().push(this);
     }
    THazardPtrManager::instance().gc();
}
