#include "tstdoutsystemlogger.h"
#include <QByteArray>
#include <iostream>


int TStdOutSystemLogger::write(const char *data, int length)
{
    std::cout << QByteArray(data, length).data();
    return 0;
}


void TStdOutSystemLogger::flush()
{
   std::cout << std::flush;
}
