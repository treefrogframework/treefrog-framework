#include "thazardpointer.h"
#include "thazardpointermanager.h"

extern THazardPointerManager hazardPointerManager;


THazardPointer::THazardPointer()
    : rec(new THazardPointerRecord())
{
    hazardPointerManager.push(rec);
}


THazardPointer::~THazardPointer()
{
    rec->hazptr.store((const THazardObject*)0x01, std::memory_order_release);
}


void THazardPointer::set(const THazardObject *ptr)
{
    rec->hazptr.store(ptr, std::memory_order_release);
}


void THazardPointer::clear()
{
    rec->hazptr.store(nullptr, std::memory_order_release);
}


void THazardPointer::swap(THazardPointer &, THazardPointer &)
{
    // TODO
    // TODO
}
