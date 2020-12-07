//
// EnlyzeWinCompatLib - Let MSVC v141_xp targeted applications run on even older Windows versions
// Written by Colin Finck for ENLYZE GmbH
//

// This file implements required APIs not available in Windows 2000 RTM (NT 5.0).
#if !defined(_WIN64)
#include "EnlyzeWinCompatLibInternal.h"

typedef PVOID (WINAPI *PFN_DECODEPOINTER)(PVOID Ptr);
typedef PVOID (WINAPI *PFN_ENCODEPOINTER)(PVOID Ptr);

static PFN_DECODEPOINTER pfnDecodePointer = nullptr;
static PFN_ENCODEPOINTER pfnEncodePointer = nullptr;

static PVOID WINAPI
_DummyDecodePointer(PVOID Ptr)
{
    // Just return the input pointer without any decoding.
    return Ptr;
}

static PVOID WINAPI
_DummyEncodePointer(PVOID Ptr)
{
    // Just return the input pointer without any encoding.
    return Ptr;
}

extern "C" PVOID WINAPI
_imp__DecodePointer(PVOID Ptr)
{
    if (!pfnDecodePointer)
    {
        // Check if the API is provided by kernel32, otherwise fall back to our dummy implementation.
        HMODULE hKernel32 = GetModuleHandleW(L"kernel32");
        pfnDecodePointer = reinterpret_cast<PFN_DECODEPOINTER>(GetProcAddress(hKernel32, "DecodePointer"));
        if (!pfnDecodePointer)
        {
            pfnDecodePointer = _DummyDecodePointer;
        }
    }

    return pfnDecodePointer(Ptr);
}

extern "C" PVOID WINAPI
_imp__EncodePointer(PVOID Ptr)
{
    if (!pfnEncodePointer)
    {
        // Check if the API is provided by kernel32, otherwise fall back to our dummy implementation.
        HMODULE hKernel32 = GetModuleHandleW(L"kernel32");
        pfnEncodePointer = reinterpret_cast<PFN_ENCODEPOINTER>(GetProcAddress(hKernel32, "EncodePointer"));
        if (!pfnEncodePointer)
        {
            pfnEncodePointer = _DummyEncodePointer;
        }
    }

    return pfnEncodePointer(Ptr);
}

#endif
