/* Copyright (c) 2011-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "processinfo.h"
#include <QtCore>
#include <TWebApplication>
#include <Windows.h>
#include <ntstatus.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <winternl.h>

namespace TreeFrog {


bool ProcessInfo::exists() const
{
    return ProcessInfo::allConcurrentPids().contains(processId);
}


int64_t ProcessInfo::ppid() const
{
    DWORD pidParent = 0;
    HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapShot == INVALID_HANDLE_VALUE) {
        return pidParent;
    }

    PROCESSENTRY32 procentry;
    ZeroMemory((LPVOID)&procentry, sizeof(PROCESSENTRY32));
    procentry.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapShot, &procentry)) {
        do {
            if (procentry.th32ProcessID == (DWORD)processId) {
                pidParent = procentry.th32ParentProcessID;
                break;
            }
            procentry.dwSize = sizeof(PROCESSENTRY32);
        } while (Process32Next(hSnapShot, &procentry));
    }

    CloseHandle(hSnapShot);
    return pidParent;
}


QString ProcessInfo::processName() const
{
    QString ret;
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (hProcess) {
        WCHAR fileName[512];
        DWORD len = GetModuleFileNameEx(hProcess, NULL, (LPWSTR)fileName, 512);
        if (len > 0) {
            QString path = QString::fromUtf16((ushort *)fileName);
            ret = QFileInfo(path).baseName();
        }
        CloseHandle(hProcess);
    }
    return ret;
}


void ProcessInfo::terminate()
{
    if (processId > 0) {
        // Sends to the local socket of tfmanager
        TWebApplication::sendLocalCtrlMessage(QByteArray::number(WM_CLOSE), processId);
    }
}


void ProcessInfo::kill()
{
    if (processId > 0) {
        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, (DWORD)processId);
        if (hProcess) {
            TerminateProcess(hProcess, 0);
            WaitForSingleObject(hProcess, 500);
            CloseHandle(hProcess);
        }
    }
    processId = -1;
}


void ProcessInfo::restart()
{
    if (processId > 0) {
        // Sends to the local socket of tfmanager
        TWebApplication::sendLocalCtrlMessage(QByteArray::number(WM_APP), processId);
    }
}


QList<int64_t> ProcessInfo::allConcurrentPids()
{
    QList<int64_t> ret;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 entry;

    entry.dwSize = sizeof(PROCESSENTRY32);
    if (Process32First(hSnapshot, &entry)) {
        do {
            ret << (int64_t)entry.th32ProcessID;
        } while (Process32Next(hSnapshot, &entry));
    }
    CloseHandle(hSnapshot);

    std::sort(ret.begin(), ret.end());  // Sorts the items
    return ret;
}

}  // namespace TreeFrog
