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
    rec->hazptr.store((THazardObject*)0x01);
    hazardPointerManager.gc();
}


void THazardPointer::guard(THazardObject *ptr)
{
    rec->hazptr.store((THazardObject*)((quintptr)ptr & ~Mask));
}


void THazardPointer::clear()
{
    rec->hazptr.store(nullptr);
}
