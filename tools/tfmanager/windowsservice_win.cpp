/* Copyright (c) 2012-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QtCore>
#include <TGlobal>
#include <TSystemGlobal>
#include <TWebApplication>
#include <vector>
#include <Windows.h>
namespace TreeFrog {

extern int managerMain(int argc, char *argv[]);

static SERVICE_STATUS_HANDLE statusHandle;

static SERVICE_STATUS serviceStatus = {
    SERVICE_WIN32_OWN_PROCESS,  // dwServiceType;
    SERVICE_START_PENDING,  // dwCurrentState
    SERVICE_ACCEPT_STOP,  // dwControlsAccepted
    NO_ERROR,  // dwWin32ExitCode
    NO_ERROR,  // dwServiceSpecificExitCode
    0,  // dwCheckPoint
    0  // dwWaitHint
};


static QString getServiceName(DWORD processId)
{
    QString name;
    SC_HANDLE hSCM = OpenSCManager(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE | SC_MANAGER_CONNECT);
    if (!hSCM) {
        return name;
    }

    DWORD bufSize = 0;
    DWORD servCount = 0;
    EnumServicesStatusEx(hSCM, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL,
        nullptr, 0, &bufSize, &servCount, nullptr, nullptr);

    std::vector<BYTE> buffer(bufSize);
    EnumServicesStatusEx(hSCM, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL,
        buffer.data(), (DWORD)buffer.size(), &bufSize,
        &servCount, nullptr, nullptr);

    auto services = reinterpret_cast<LPENUM_SERVICE_STATUS_PROCESS>(buffer.data());
    for (unsigned int i = 0; i < servCount; ++i) {
        ENUM_SERVICE_STATUS_PROCESS service = services[i];
        if (service.ServiceStatusProcess.dwProcessId == processId) {
            // This is your service.
            name = QString::fromUtf16((const ushort *)service.lpServiceName);
            break;
        }
    }
    CloseServiceHandle(hSCM);
    return name;
}


static QString getServiceFilePath(const QString &serviceName)
{
    QString result;
    // Open the Service Control Manager
    SC_HANDLE schSCManager = OpenSCManager(nullptr,  // local computer
        nullptr,  // ServicesActive database
        SC_MANAGER_ALL_ACCESS);  // full access rights
    if (schSCManager) {
        // Try to open the service
        SC_HANDLE schService = OpenService(schSCManager, (const wchar_t *)serviceName.utf16(), SERVICE_QUERY_CONFIG);
        if (schService) {
            DWORD sizeNeeded = 0;
            char data[8 * 1024];
            if (QueryServiceConfig(schService, (LPQUERY_SERVICE_CONFIG)data, sizeof(data), &sizeNeeded)) {
                LPQUERY_SERVICE_CONFIG config = (LPQUERY_SERVICE_CONFIG)data;
                result = QString::fromUtf16((const ushort *)config->lpBinaryPathName);
            }
            CloseServiceHandle(schService);
        }
        CloseServiceHandle(schSCManager);
    }
    return result;
}


static void WINAPI serviceHandler(DWORD ctrl)
{
    switch (ctrl) {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        tSystemInfo("Windows service: Received a stop-service request.");
        serviceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        serviceStatus.dwWaitHint = 30000;
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


static QByteArrayList parseArguments(const QString &str)
{
    const QRegularExpression re("(\"([^\"]*)\"|([^ ]+))", QRegularExpression::CaseInsensitiveOption);

    QByteArrayList res;
    int pos = 0;

    for (;;) {
        auto match = re.match(str, pos);
        if (!match.hasMatch()) {
            break;
        }
        QString cap2 = match.captured(2);
        res << ((cap2.isEmpty()) ? match.captured(3) : cap2).toLocal8Bit();
        pos += match.capturedLength();
    }
    return res;
}


void WINAPI winServiceMain(DWORD, LPTSTR *)
{
    auto serviceName = getServiceName(GetCurrentProcessId());
    statusHandle = RegisterServiceCtrlHandler((const wchar_t *)serviceName.utf16(), serviceHandler);
    if (!statusHandle) {
        return;
    }

    // Service status
    serviceStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(statusHandle, &serviceStatus);

    // Main function
    int ret = 1;
    QString pathstr = getServiceFilePath(serviceName);
    if (!pathstr.isEmpty()) {
        auto argList = parseArguments(pathstr);
        auto *args = new char *[argList.count()];

        for (int i = 0; i < argList.count(); i++) {
            args[i] = argList[i].data();
        }
        ret = managerMain(argList.count(), args);
        delete[] args;
    }

    // Cleanup code must be executed before setting status to SERVICE_STOPPED
    tSystemInfo("Windows service stopped");
    Tf::releaseSystemLogger();

    // Service status
    serviceStatus.dwCurrentState = SERVICE_STOPPED;
    serviceStatus.dwWin32ExitCode = ret;
    SetServiceStatus(statusHandle, &serviceStatus);
}

}  // namespace TreeFrog
