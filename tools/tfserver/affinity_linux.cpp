#include "affinity.h"
#include <TGlobal>
#include <thread>  // for hardware_concurrency()
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sched.h>


void Tf::setCpuAffinity(int cpu)
{
    cpu_set_t mask;
    int cpunum = qMax(std::thread::hardware_concurrency(), (uint)1);

    if (cpu < 0 || cpunum == 1) {
        return;
    }

    cpu %= cpunum;
    CPU_ZERO(&mask);
    CPU_SET(cpu, &mask);
    sched_setaffinity(0, sizeof(cpu_set_t), &mask);
}
