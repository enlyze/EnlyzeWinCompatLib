//
// EnlyzeWinCompatLib - Let Clang-compiled applications run on older Windows versions
// Copyright (c) 2021 Colin Finck, ENLYZE GmbH <c.finck@enlyze.com>
// SPDX-License-Identifier: MIT
//

// This file implements required APIs not available in Windows NT 4.0 RTM.
#include "EnlyzeWinCompatLibInternal.h"

typedef BOOL (WINAPI *PFN_GETFILESIZEEX)(HANDLE hFile, PLARGE_INTEGER lpFileSize);
typedef BOOL (WINAPI *PFN_INITIALIZECRITICALSECTIONANDSPINCOUNT)(LPCRITICAL_SECTION lpCriticalSection, DWORD dwSpinCount);
typedef BOOL (WINAPI *PFN_SETFILEPOINTEREX)(HANDLE hFile, LARGE_INTEGER liDistanceToMove, PLARGE_INTEGER lpNewFilePointer, DWORD dwMoveMethod);

static PFN_GETFILESIZEEX pfnGetFileSizeEx = nullptr;
static PFN_INITIALIZECRITICALSECTIONANDSPINCOUNT pfnInitializeCriticalSectionAndSpinCount = nullptr;
static PFN_SETFILEPOINTEREX pfnSetFilePointerEx = nullptr;

static BOOL WINAPI
_CompatGetFileSizeEx(HANDLE hFile, PLARGE_INTEGER lpFileSize)
{
    lpFileSize->LowPart = GetFileSize(hFile, reinterpret_cast<LPDWORD>(&lpFileSize->HighPart));
    if (lpFileSize->LowPart == INVALID_FILE_SIZE && GetLastError() != NO_ERROR)
    {
        return FALSE;
    }

    return TRUE;
}

static BOOL WINAPI
_CompatInitializeCriticalSectionAndSpinCount(LPCRITICAL_SECTION lpCriticalSection, DWORD dwSpinCount)
{
    UNREFERENCED_PARAMETER(dwSpinCount);

    // The information in dwSpinCount is only a performance optimization for multiprocessor systems.
    // If an operating system doesn't have it, it's fine to just call InitializeCriticalSection.
    InitializeCriticalSection(lpCriticalSection);

    return TRUE;
}

static BOOL WINAPI
_CompatSetFilePointerEx(HANDLE hFile, LARGE_INTEGER liDistanceToMove, PLARGE_INTEGER lpNewFilePointer, DWORD dwMoveMethod)
{
    // This function can be implemented in its entirety using SetFilePointer.
    liDistanceToMove.LowPart = SetFilePointer(hFile, liDistanceToMove.LowPart, &liDistanceToMove.HighPart, dwMoveMethod);
    if (liDistanceToMove.LowPart == INVALID_SET_FILE_POINTER)
    {
        return FALSE;
    }

    if (lpNewFilePointer)
    {
        lpNewFilePointer->QuadPart = liDistanceToMove.QuadPart;
    }

    return TRUE;
}

extern "C" BOOL WINAPI
LibGetFileSizeEx(HANDLE hFile, PLARGE_INTEGER lpFileSize)
{
    if (!pfnGetFileSizeEx)
    {
        // Check if the API is provided by kernel32, otherwise fall back to our implementation.
        HMODULE hKernel32 = GetModuleHandleW(L"kernel32");
        pfnGetFileSizeEx = reinterpret_cast<PFN_GETFILESIZEEX>(GetProcAddress(hKernel32, "GetFileSizeEx"));
        if (!pfnGetFileSizeEx)
        {
            pfnGetFileSizeEx = _CompatGetFileSizeEx;
        }
    }

    return pfnGetFileSizeEx(hFile, lpFileSize);
}

extern "C" BOOL WINAPI
LibInitializeCriticalSectionAndSpinCount(LPCRITICAL_SECTION lpCriticalSection, DWORD dwSpinCount)
{
    if (!pfnInitializeCriticalSectionAndSpinCount)
    {
        // Check if the API is provided by kernel32, otherwise fall back to our implementation.
        HMODULE hKernel32 = GetModuleHandleW(L"kernel32");
        pfnInitializeCriticalSectionAndSpinCount = reinterpret_cast<PFN_INITIALIZECRITICALSECTIONANDSPINCOUNT>(GetProcAddress(hKernel32, "InitializeCriticalSectionAndSpinCount"));
        if (!pfnInitializeCriticalSectionAndSpinCount)
        {
            pfnInitializeCriticalSectionAndSpinCount = _CompatInitializeCriticalSectionAndSpinCount;
        }
    }

    return pfnInitializeCriticalSectionAndSpinCount(lpCriticalSection, dwSpinCount);
}

extern "C" BOOL WINAPI
LibSetFilePointerEx(HANDLE hFile, LARGE_INTEGER liDistanceToMove, PLARGE_INTEGER lpNewFilePointer, DWORD dwMoveMethod)
{
    if (!pfnSetFilePointerEx)
    {
        // Check if the API is provided by kernel32, otherwise fall back to our implementation.
        HMODULE hKernel32 = GetModuleHandleW(L"kernel32");
        pfnSetFilePointerEx = reinterpret_cast<PFN_SETFILEPOINTEREX>(GetProcAddress(hKernel32, "SetFilePointerEx"));
        if (!pfnSetFilePointerEx)
        {
            pfnSetFilePointerEx = _CompatSetFilePointerEx;
        }
    }

    return pfnSetFilePointerEx(hFile, liDistanceToMove, lpNewFilePointer, dwMoveMethod);
}
