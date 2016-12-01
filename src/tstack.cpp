#include "tstack.h"
#include <QThreadStorage>

static QThreadStorage<THazardPtr> hzptrTls;


THazardPtr &Tf::hazardPtrForStack()
{
    return hzptrTls.localData();
}
