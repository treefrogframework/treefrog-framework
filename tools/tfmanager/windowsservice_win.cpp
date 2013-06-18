/* Copyright (c) 2012-2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <Windows.h>
#include <TGlobal>
#include <TWebApplication>
#include <TSystemGlobal>
namespace TreeFrog {

extern int managerMain(int argc, char *argv[]);

static SERVICE_STATUS_HANDLE statusHandle;

static SERVICE_STATUS serviceStatus = {
    SERVICE_WIN32_OWN_PROCESS,   // dwServiceType;
    SERVICE_START_PENDING,   // dwCurrentState
    SERVICE_ACCEPT_STOP,     // dwControlsAccepted
    NO_ERROR,   // dwWin32ExitCode
    NO_ERROR,   // dwServiceSpecificExitCode
    0,          // dwCheckPoint
    0           // dwWaitHint
};


static void WINAPI serviceHandler(DWORD ctrl)
{
    switch (ctrl) {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        tSystemInfo("Windows service: Received a stop-service request.");
        serviceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        serviceStatus.dwWaitHint     = 30000;
        SetServiceStatus(statusHandle, &serviceStatus);
        Tf::app()->exit(0);
        break;
        
    case SERVICE_CONTROL_PAUSE:
    case SERVICE_CONTROL_CONTINUE:
    case SERVICE_CONTROL_INTERROGATE:
        tSystemWarn("Windows service: Received ctrl code: %ld ", ctrl);
        SetServiceStatus(statusHandle, &serviceStatus);
        break;
        
    default:
        tSystemWarn("Windows service: Invalid ctrl code: %ld ", ctrl);
        break;
    }
}


void WINAPI winServiceMain(DWORD, LPTSTR *)
{
    statusHandle = RegisterServiceCtrlHandler(TEXT("treefrogs"), serviceHandler);
    if (!statusHandle)
        return;
    
    // Service status
    serviceStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(statusHandle, &serviceStatus);

    // Main function
    int ret = managerMain(0, (char**)"");

    // Service status
    serviceStatus.dwCurrentState = SERVICE_STOPPED;
    serviceStatus.dwWin32ExitCode = ret;
    SetServiceStatus(statusHandle, &serviceStatus);
}

} // namespace TreeFrog
