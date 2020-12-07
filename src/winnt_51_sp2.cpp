//
// EnlyzeWinCompatLib - Let MSVC v141_xp targeted applications run on even older Windows versions
// Written by Colin Finck for ENLYZE GmbH
//

// This file implements required APIs not available in Windows XP SP2 (NT 5.1).
#if !defined(_WIN64)
#include "EnlyzeWinCompatLibInternal.h"

typedef BOOL (WINAPI *PFN_GETLOGICALPROCESSORINFORMATION)(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION Buffer, PDWORD ReturnedLength);

static PFN_GETLOGICALPROCESSORINFORMATION pfnGetLogicalProcessorInformation = nullptr;

static BOOL WINAPI
_DummyGetLogicalProcessorInformation(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION Buffer, PDWORD ReturnedLength)
{
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(ReturnedLength);

    // This function is not meant to ever be called on systems that don't support it.
    // The CRT checks the OS version at runtime and doesn't call it for XP, but this still introduces a
    // static dependency to GetLogicalProcessorInformation on all platforms.
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

extern "C" BOOL WINAPI
_imp__GetLogicalProcessorInformation(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION Buffer, PDWORD ReturnedLength)
{
    if (!pfnGetLogicalProcessorInformation)
    {
        // Check if the API is provided by kernel32, otherwise fall back to our dummy implementation.
        HMODULE hKernel32 = GetModuleHandleW(L"kernel32");
        pfnGetLogicalProcessorInformation = reinterpret_cast<PFN_GETLOGICALPROCESSORINFORMATION>(GetProcAddress(hKernel32, "GetLogicalProcessorInformation"));
        if (!pfnGetLogicalProcessorInformation)
        {
            pfnGetLogicalProcessorInformation = _DummyGetLogicalProcessorInformation;
        }
    }

    return pfnGetLogicalProcessorInformation(Buffer, ReturnedLength);
}

#endif
