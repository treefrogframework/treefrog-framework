#include "tstack.h"
#include <thread>


THazardPtr &Tf::hazardPtrForStack()
{
    static thread_local THazardPtr hzptrTls;
    return hzptrTls;
}
