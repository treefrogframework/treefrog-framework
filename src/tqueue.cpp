#include "tqueue.h"
#include <QThreadStorage>

static QThreadStorage<THazardPtr> hzptrTls;


THazardPtr &Tf::hazardPtrForQueue()
{
    return hzptrTls.localData();
}
