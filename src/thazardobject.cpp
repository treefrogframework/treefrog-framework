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
    if (!deleted.exchange(true)) {
        THazardPtrManager::instance().push(this);
    }
    THazardPtrManager::instance().gc();
}
