#include "tqueue.h"
#include <thread>


THazardPtr &Tf::hazardPtrForQueue()
{
    static thread_local THazardPtr hzptrTls;
    return hzptrTls;
}
