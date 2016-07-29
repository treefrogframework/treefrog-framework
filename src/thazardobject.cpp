#include "thazardobject.h"
#include "thazardpointermanager.h"

extern THazardPointerManager hazardPointerManager;


void THazardObject::deleteLater()
{
    bool False = false;
    if (deleted.compare_exchange_strong(False, true)) {
        hazardPointerManager.gc();
        hazardPointerManager.push(this);
    }
}
