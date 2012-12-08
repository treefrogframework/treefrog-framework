/* Copyright (c) 2010, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QAtomicPointer>
#include <TLogWriter>
#include <TWebApplication>
#include "tlogfilewriter.h"
#include "tstandardoutputwriter.h"

static QAtomicPointer<TAbstractLogWriter> writer = 0;


static void cleanup()
{
    if ((TAbstractLogWriter *)writer) {
        delete (TAbstractLogWriter *)writer;
        writer = 0;
    }
}


void TLogWriter::write(const char *msg)
{
    TAbstractLogWriter *lw = (TAbstractLogWriter *)writer;
    if (!lw) {
        TWebApplication::MultiProcessingModule mpm = tWebApp->multiProcessingModule();

        if (mpm == TWebApplication::Prefork) {
            lw = new TStandardOutputWriter;
        } else if (mpm == TWebApplication::Thread) {
            lw = new TLogFileWriter(tWebApp->logFilePath());
        } else {
            printf("Invalid MPM specified\n");
            return;
        }

        if (writer.testAndSetOrdered(0, lw)) {
            qAddPostRoutine(cleanup);
        } else {
            delete lw;
            lw = (TAbstractLogWriter *)writer;
        }
    }

    lw->writeLog(msg);
}
