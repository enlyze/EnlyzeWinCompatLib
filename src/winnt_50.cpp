//
// EnlyzeWinCompatLib - Let Clang-compiled applications run on older Windows versions
// Copyright (c) 2021 Colin Finck, ENLYZE GmbH <c.finck@enlyze.com>
// SPDX-License-Identifier: MIT
//

// This file implements required APIs not available in Windows 2000 RTM (NT 5.0).
#include "EnlyzeWinCompatLibInternal.h"

typedef struct _COMPAT_SLIST_ENTRY
{
    struct _COMPAT_SLIST_ENTRY* Next;
}
COMPAT_SLIST_ENTRY, *PCOMPAT_SLIST_ENTRY;

typedef union _COMPAT_SLIST_HEADER
{
    ULONGLONG Alignment;
    struct
    {
        COMPAT_SLIST_ENTRY Next;
        WORD Depth;
        WORD Sequence;
    };
}
COMPAT_SLIST_HEADER, *PCOMPAT_SLIST_HEADER;

typedef BOOL (WINAPI *PFN_GETMODULEHANDLEEXW)(DWORD dwFlags, LPCWSTR lpModuleName, HMODULE* phModule);
typedef BOOL (WINAPI *PFN_HEAPQUERYINFORMATION)(HANDLE HeapHandle, HEAP_INFORMATION_CLASS HeapInformationClass, PVOID HeapInformation, SIZE_T HeapInformationLength, PSIZE_T ReturnLength);
typedef void (WINAPI *PFN_INITIALIZESLISTHEAD)(PCOMPAT_SLIST_HEADER ListHead);
typedef PCOMPAT_SLIST_ENTRY (WINAPI *PFN_INTERLOCKEDFLUSHSLIST)(PCOMPAT_SLIST_HEADER ListHead);
typedef PCOMPAT_SLIST_ENTRY (WINAPI *PFN_INTERLOCKEDPUSHENTRYSLIST)(PCOMPAT_SLIST_HEADER ListHead, PCOMPAT_SLIST_ENTRY ListEntry);

static PFN_GETMODULEHANDLEEXW pfnGetModuleHandleExW = nullptr;
static PFN_HEAPQUERYINFORMATION pfnHeapQueryInformation = nullptr;
static PFN_INITIALIZESLISTHEAD pfnInitializeSListHead = nullptr;
static PFN_INTERLOCKEDFLUSHSLIST pfnInterlockedFlushSList = nullptr;
static PFN_INTERLOCKEDPUSHENTRYSLIST pfnInterlockedPushEntrySList = nullptr;

static BOOL WINAPI
_CompatGetModuleHandleExW(DWORD dwFlags, LPCWSTR lpModuleName, HMODULE* phModule)
{
    UNREFERENCED_PARAMETER(dwFlags);
    UNREFERENCED_PARAMETER(lpModuleName);
    UNREFERENCED_PARAMETER(phModule);

    // This function is only called once by try_cor_exit_process when exiting the process.
    // It checks whether this is a .NET process (lpModuleName = "mscoree.dll") to call cor_exit_process just in case.
    // As we know that we are not a .NET process, we can just return FALSE here.
    return FALSE;
}

static BOOL WINAPI
_CompatHeapQueryInformation(HANDLE HeapHandle, HEAP_INFORMATION_CLASS HeapInformationClass, PVOID HeapInformation, SIZE_T HeapInformationLength, PSIZE_T ReturnLength)
{
    // We assume that a correct handle has been passed and that HeapCompatibilityInformation
    // (as the only supported HeapInformationClass) has been queried.
    UNREFERENCED_PARAMETER(HeapHandle);
    UNREFERENCED_PARAMETER(HeapInformationClass);

    if (HeapInformationLength < sizeof(ULONG))
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    // An operating system so old that it needs this implementation doesn't support any fancy heap features.
    // So return 0 to indicate a standard heap here.
    PULONG pCompatibilityInformation = reinterpret_cast<PULONG>(HeapInformation);
    *pCompatibilityInformation = 0;

    if (ReturnLength)
    {
        *ReturnLength = sizeof(ULONG);
    }

    return TRUE;
}

// Verified to functionally match the ReactOS implementation on i386.
static void WINAPI
_CompatInitializeSListHead(PCOMPAT_SLIST_HEADER ListHead)
{
    // Due to the union, this member encompasses the entire SLIST_HEADER structure.
    ListHead->Alignment = 0;
}

// Verified to functionally match the ReactOS implementation on i386.
static PCOMPAT_SLIST_ENTRY WINAPI
_CompatInterlockedFlushSList(PCOMPAT_SLIST_HEADER ListHead)
{
    COMPAT_SLIST_HEADER InitialHead = *ListHead;

    for (;;)
    {
        // Check if the list is already empty.
        if (InitialHead.Next.Next == nullptr)
        {
            // Nothing to do for us and no list entry to return.
            return nullptr;
        }

        // Zero the depth and first list entry pointer while keeping the sequence number intact.
        COMPAT_SLIST_HEADER NewHead = InitialHead;
        NewHead.Depth = 0;
        NewHead.Next.Next = nullptr;

        // Do the following things in a single atomic operation:
        // * Check if the list is still the same we last saved in InitialHead.
        // * If that is the case, update it to our modified NewHead.
        // * If not, someone else was faster than us. Store the changed list in PreviousHead.
        COMPAT_SLIST_HEADER PreviousHead;
        PreviousHead.Alignment = static_cast<ULONGLONG>(_InterlockedCompareExchange64(
            reinterpret_cast<long long*>(&ListHead->Alignment),
            static_cast<long long>(NewHead.Alignment),
            static_cast<long long>(InitialHead.Alignment)
        ));

        if (InitialHead.Alignment == PreviousHead.Alignment)
        {
            // We successfully flushed the list.
            // Return the previous first list entry.
            return PreviousHead.Next.Next;
        }
        else
        {
            // Someone else was faster than us.
            // Repeat the entire process with the new InitialHead.
            InitialHead = PreviousHead;
        }
    }
}

// Verified to functionally match the ReactOS implementation on i386.
static PCOMPAT_SLIST_ENTRY WINAPI
_CompatInterlockedPushEntrySList(PCOMPAT_SLIST_HEADER ListHead, PCOMPAT_SLIST_ENTRY ListEntry)
{
    COMPAT_SLIST_HEADER InitialHead = *ListHead;

    for (;;)
    {
        // Mount the list entry at the very beginning and adjust depth and sequence.
        COMPAT_SLIST_HEADER NewHead = InitialHead;
        ListEntry->Next = NewHead.Next.Next;
        NewHead.Next.Next = ListEntry;
        NewHead.Depth++;
        NewHead.Sequence++;

        // Do the following things in a single atomic operation:
        // * Check if the list is still the same we last saved in InitialHead.
        // * If that is the case, update it to our modified NewHead.
        // * If not, someone else was faster than us. Store the changed list in PreviousHead.
        COMPAT_SLIST_HEADER PreviousHead;
        PreviousHead.Alignment = static_cast<ULONGLONG>(_InterlockedCompareExchange64(
            reinterpret_cast<long long*>(&ListHead->Alignment),
            static_cast<long long>(NewHead.Alignment),
            static_cast<long long>(InitialHead.Alignment)
        ));

        if (InitialHead.Alignment == PreviousHead.Alignment)
        {
            // We successfully pushed to the list.
            // Return the previous first list entry.
            return PreviousHead.Next.Next;
        }
        else
        {
            // Someone else was faster than us.
            // Repeat the entire process with the new InitialHead.
            InitialHead = PreviousHead;
        }
    }
}

extern "C" BOOL WINAPI
LibGetModuleHandleExW(DWORD dwFlags, LPCWSTR lpModuleName, HMODULE * phModule)
{
    if (!pfnGetModuleHandleExW)
    {
        // Check if the API is provided by kernel32, otherwise fall back to our implementation.
        HMODULE hKernel32 = GetModuleHandleW(L"kernel32");
        pfnGetModuleHandleExW = reinterpret_cast<PFN_GETMODULEHANDLEEXW>(GetProcAddress(hKernel32, "GetModuleHandleExW"));
        if (!pfnGetModuleHandleExW)
        {
            pfnGetModuleHandleExW = _CompatGetModuleHandleExW;
        }
    }

    return pfnGetModuleHandleExW(dwFlags, lpModuleName, phModule);
}

extern "C" BOOL WINAPI
LibHeapQueryInformation(HANDLE HeapHandle, HEAP_INFORMATION_CLASS HeapInformationClass, PVOID HeapInformation, SIZE_T HeapInformationLength, PSIZE_T ReturnLength)
{
    if (!pfnHeapQueryInformation)
    {
        // Check if the API is provided by kernel32, otherwise fall back to our implementation.
        HMODULE hKernel32 = GetModuleHandleW(L"kernel32");
        pfnHeapQueryInformation = reinterpret_cast<PFN_HEAPQUERYINFORMATION>(GetProcAddress(hKernel32, "HeapQueryInformation"));
        if (!pfnHeapQueryInformation)
        {
            pfnHeapQueryInformation = _CompatHeapQueryInformation;
        }
    }

    return pfnHeapQueryInformation(HeapHandle, HeapInformationClass, HeapInformation, HeapInformationLength, ReturnLength);
}

extern "C" void WINAPI
LibInitializeSListHead(PCOMPAT_SLIST_HEADER ListHead)
{
    if (!pfnInitializeSListHead)
    {
        // Check if the API is provided by kernel32, otherwise fall back to our implementation.
        HMODULE hKernel32 = GetModuleHandleW(L"kernel32");
        pfnInitializeSListHead = reinterpret_cast<PFN_INITIALIZESLISTHEAD>(GetProcAddress(hKernel32, "InitializeSListHead"));
        if (!pfnInitializeSListHead)
        {
            pfnInitializeSListHead = _CompatInitializeSListHead;
        }
    }

    return pfnInitializeSListHead(ListHead);
}

extern "C" PCOMPAT_SLIST_ENTRY WINAPI
LibInterlockedFlushSList(PCOMPAT_SLIST_HEADER ListHead)
{
    if (!pfnInterlockedFlushSList)
    {
        // Check if the API is provided by kernel32, otherwise fall back to our implementation.
        HMODULE hKernel32 = GetModuleHandleW(L"kernel32");
        pfnInterlockedFlushSList = reinterpret_cast<PFN_INTERLOCKEDFLUSHSLIST>(GetProcAddress(hKernel32, "InterlockedFlushSList"));
        if (!pfnInterlockedFlushSList)
        {
            pfnInterlockedFlushSList = _CompatInterlockedFlushSList;
        }
    }

    return pfnInterlockedFlushSList(ListHead);
}

extern "C" PCOMPAT_SLIST_ENTRY WINAPI
LibInterlockedPushEntrySList(PCOMPAT_SLIST_HEADER ListHead, PCOMPAT_SLIST_ENTRY ListEntry)
{
    if (!pfnInterlockedPushEntrySList)
    {
        // Check if the API is provided by kernel32, otherwise fall back to our implementation.
        HMODULE hKernel32 = GetModuleHandleW(L"kernel32");
        pfnInterlockedPushEntrySList = reinterpret_cast<PFN_INTERLOCKEDPUSHENTRYSLIST>(GetProcAddress(hKernel32, "InterlockedPushEntrySList"));
        if (!pfnInterlockedPushEntrySList)
        {
            pfnInterlockedPushEntrySList = _CompatInterlockedPushEntrySList;
        }
    }

    return pfnInterlockedPushEntrySList(ListHead, ListEntry);
}
