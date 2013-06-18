/* Copyright (c) 2010-2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <iostream>
#include <TActionForkProcess>
#include <TWebApplication>
#include <TSqlDatabasePool>

/*!
  \class TActionForkProcess
  \brief The TActionForkProcess class provides a context of a
  forked process.
*/

TActionForkProcess *TActionForkProcess::currentActionContext = 0;


TActionForkProcess::TActionForkProcess(int socket)
    : QObject(), TActionContext(socket)
{ }


TActionForkProcess::~TActionForkProcess()
{
    currentActionContext = 0;
}


void TActionForkProcess::emitError(int socketError)
{
    emit error(socketError);
}


TActionForkProcess *TActionForkProcess::currentContext()
{
    return currentActionContext;
}


void TActionForkProcess::start()
{
    if (currentActionContext)
        return;

    currentActionContext = this;
    std::cerr << "_accepted" << std::flush;  // send to tfmanager
    execute();

    // For cleanup
    QEventLoop eventLoop;
    while (eventLoop.processEvents()) {}

    emit finished();
    QCoreApplication::exit(1);
}
