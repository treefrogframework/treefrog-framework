/* Copyright (c) 2010-2012, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QEventLoop>
#include <TActionThread>
#include <TSqlDatabasePool>

/*!
  \class TActionThread
  \brief The TActionThread class provides a thread context.
*/


TActionThread::TActionThread(int socket)
    : QThread(), TActionContext(socket)
{ }


TActionThread::~TActionThread()
{ }


void TActionThread::run()
{
    execute();

    // For cleanup
    QEventLoop eventLoop;
    while (eventLoop.processEvents()) {}
}


void TActionThread::emitError(int socketError)
{
    emit error(socketError);
}
