#include "tstdoutsystemlogger.h"
#include <QByteArray>
#include <iostream>


int TStdoutSystemLogger::write(const char *data, int length)
{
    std::cout << QByteArray(data, length).data();
    return 0;
}


void TStdoutSystemLogger::flush()
{
   std::cout << std::flush;
}
