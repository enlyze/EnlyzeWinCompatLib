//
// EnlyzeWinCompatLib - Let MSVC v141_xp targeted applications run on even older Windows versions
// Written by Colin Finck for ENLYZE GmbH
//

// This file implements required APIs not available in Windows NT 4.0 RTM.
#include "EnlyzeWinCompatLibInternal.h"

typedef void (WINAPI *PFN_INITIALIZECRITICALSECTIONANDSPINCOUNT)(LPCRITICAL_SECTION lpCriticalSection, DWORD dwSpinCount);
typedef BOOL (WINAPI *PFN_SETFILEPOINTEREX)(HANDLE hFile, LARGE_INTEGER liDistanceToMove, PLARGE_INTEGER lpNewFilePointer, DWORD dwMoveMethod);

static PFN_INITIALIZECRITICALSECTIONANDSPINCOUNT pfnInitializeCriticalSectionAndSpinCount = nullptr;
static PFN_SETFILEPOINTEREX pfnSetFilePointerEx = nullptr;

static void WINAPI
_CompatInitializeCriticalSectionAndSpinCount(LPCRITICAL_SECTION lpCriticalSection, DWORD dwSpinCount)
{
    UNREFERENCED_PARAMETER(dwSpinCount);

    // The information in dwSpinCount is only a performance optimization for multiprocessor systems.
    // If an operating system doesn't have it, it's fine to just call InitializeCriticalSection.
    InitializeCriticalSection(lpCriticalSection);
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

extern "C" void WINAPI
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

    pfnInitializeCriticalSectionAndSpinCount(lpCriticalSection, dwSpinCount);
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
