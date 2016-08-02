#include "thazardobject.h"
#include "thazardptrmanager.h"

extern THazardPtrManager hazardPtrManager;


THazardObject::THazardObject()
{
    hazardPtrManager.gc();
}


THazardObject::THazardObject(const THazardObject &)
{
    hazardPtrManager.gc();
}


void THazardObject::deleteLater()
{
    if (!deleted.test_and_set(std::memory_order_acquire)) {
        hazardPtrManager.push(this);
     }
    hazardPtrManager.gc();
}
