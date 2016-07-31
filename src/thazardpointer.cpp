#include "thazardpointer.h"
#include "thazardpointermanager.h"

extern THazardPointerManager hazardPointerManager;


THazardPointer::THazardPointer()
    : rec(new THazardPointerRecord())
{
    hazardPointerManager.push(rec);
    //hazardPointerManager.gcHazardPointers();
    hazardPointerManager.gc();
}


THazardPointer::~THazardPointer()
{
#if 1
    //rec->hazptr.store((const THazardObject*)0x01, std::memory_order_release);
    rec->hazptr.store((const THazardObject*)0x01);
#else
    rec->hazptr.fetchAndStoreRelease((THazardObject*)0x01);
#endif
    //hazardPointerManager.gcHazardPointers();
    hazardPointerManager.gc();
}


void THazardPointer::set(THazardObject *ptr)
{
#if 1
    rec->hazptr.store(ptr, std::memory_order_release);
#else
    rec->hazptr.fetchAndStoreRelease(ptr);
#endif
}


void THazardPointer::clear()
{
#if 1
    //rec->hazptr.store(nullptr, std::memory_order_release);
    rec->hazptr.store(nullptr);
#else
    rec->hazptr.fetchAndStoreRelease(nullptr);
#endif
}


void THazardPointer::swap(THazardPointer &, THazardPointer &)
{
    // TODO
    // TODO
}
