#include "tqueue.h"
#include <QThreadStorage>

namespace {
QThreadStorage<THazardPtr> hzptrTls;
}


THazardPtr &Tf::hazardPtrForQueue()
{
    return hzptrTls.localData();
}
