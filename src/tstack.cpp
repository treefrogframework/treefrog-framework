#include "tstack.h"
#include <QThreadStorage>


namespace {
QThreadStorage<THazardPtr> hzptrTls;
}


THazardPtr &Tf::hazardPtrForStack()
{
    return hzptrTls.localData();
}
