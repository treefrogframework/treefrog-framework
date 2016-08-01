#include "thazardpointer.h"
#include "thazardpointermanager.h"

extern THazardPointerManager hazardPointerManager;


THazardPointer::THazardPointer()
    : rec(new THazardPointerRecord())
{
    hazardPointerManager.push(rec);
    hazardPointerManager.gc();
}


THazardPointer::~THazardPointer()
{
    rec->hazptr.store((THazardObject*)0x01, std::memory_order_release);
    hazardPointerManager.gc();
}


void THazardPointer::guard(THazardObject *ptr)
{
    rec->hazptr.store(ptr, std::memory_order_release);
}


void THazardPointer::clear()
{
    rec->hazptr.store(nullptr, std::memory_order_release);
}
