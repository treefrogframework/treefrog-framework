#include "thazardobject.h"
#include "thazardpointermanager.h"

extern THazardPointerManager hazardPointerManager;


THazardObject::THazardObject()
{
    hazardPointerManager.gc();
}


THazardObject::THazardObject(const THazardObject &)
{
    hazardPointerManager.gc();
}


void THazardObject::deleteLater()
{
    if (!deleted.test_and_set(std::memory_order_acquire)) {
        hazardPointerManager.push(this);
     }
    hazardPointerManager.gc();
}
