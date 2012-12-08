/* Copyright (c) 2010-2012, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TWebApplication>
#include <windows.h>
#include <winuser.h>

static volatile int ctrlSignal = -1;


static BOOL WINAPI signalHandler(DWORD ctrlType)
{
    switch (ctrlType) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        ctrlSignal = ctrlType;
        break;
    default:
        return FALSE;
    }

    while (true)
        Sleep(1);
    
    return TRUE;
}


bool TWebApplication::winEventFilter(MSG *msg, long *result)
{
    if (msg->message == WM_CLOSE) {
        quit();
    } else if (msg->message == WM_APP) {
        exit(1);
    }
    return QCoreApplication::winEventFilter(msg, result);
}


void TWebApplication::watchConsoleSignal()
{
    SetConsoleCtrlHandler(signalHandler, TRUE);
    timer.start(100, this);
}


void TWebApplication::ignoreConsoleSignal()
{
    SetConsoleCtrlHandler(NULL, TRUE);
    timer.stop();
}


int TWebApplication::signalNumber()
{
    return ::ctrlSignal;
}


void TWebApplication::resetSignalNumber()
{
    ::ctrlSignal = -1;
}
