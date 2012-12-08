/* Copyright (c) 2010, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <iostream>
#include "tstandardoutputwriter.h"


void TStandardOutputWriter::writeLog(const char *msg)
{
    std::cout << msg << std::flush;
}
