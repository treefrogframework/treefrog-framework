#include "thazardptr.h"
#include "thazardptrmanager.h"


THazardPtr::THazardPtr() :
    rec(new THazardPtrRecord())
{
    THazardPtrManager::instance().push(rec);
    THazardPtrManager::instance().gc();
}


THazardPtr::~THazardPtr()
{
    rec->hazptr.store((THazardObject *)0x01);
    THazardPtrManager::instance().gc();
}


void THazardPtr::guard(THazardObject *ptr)
{
    rec->hazptr.store(ptr);
}


void THazardPtr::clear()
{
    rec->hazptr.store(nullptr);
}
