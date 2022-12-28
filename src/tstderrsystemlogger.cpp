/* Copyright (c) 2022, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tstderrsystemlogger.h"
#include <QByteArray>
#include <iostream>


int TStdErrSystemLogger::write(const char *data, int length)
{
    std::cerr << QByteArray(data, length).data();
    return 0;
}


void TStdErrSystemLogger::flush()
{
   std::cerr << std::flush;
}
