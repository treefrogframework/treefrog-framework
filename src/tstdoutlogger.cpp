/* Copyright (c) 2022, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tstdoutlogger.h"
#include <iostream>

/*!
  \class TStdOutLogger
  \brief The TStdOutLogger class provides functionality of logging to stdout.
*/


TStdOutLogger::TStdOutLogger() :
    TLogger()
{ }


void TStdOutLogger::log(const QByteArray &msg)
{
    std::cout << msg.data();
}


void TStdOutLogger::flush()
{
    std::cout << std::flush;
}
