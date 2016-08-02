#include "thazardptr.h"
#include "thazardptrmanager.h"

extern THazardPtrManager hazardPtrManager;


THazardPtr::THazardPtr()
    : rec(new THazardPtrRecord())
{
    hazardPtrManager.push(rec);
    hazardPtrManager.gc();
}


THazardPtr::~THazardPtr()
{
    rec->hazptr.store((THazardObject*)0x01);
    hazardPtrManager.gc();
}


void THazardPtr::guard(THazardObject *ptr)
{
    rec->hazptr.store((THazardObject*)((quintptr)ptr & ~Mask));
}


void THazardPtr::clear()
{
    rec->hazptr.store(nullptr);
}
