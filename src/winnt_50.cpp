//
// EnlyzeWinCompatLib - Let MSVC v141_xp targeted applications run on even older Windows versions
// Written by Colin Finck for ENLYZE GmbH
//

// This file implements required APIs not available in Windows NT 4.0 RTM.
#if !defined(_WIN64)
#include "EnlyzeWinCompatLibInternal.h"

typedef BOOL (WINAPI *PFN_GETMODULEHANDLEEXW)(DWORD dwFlags, LPCWSTR lpModuleName, HMODULE* phModule);
typedef BOOL (WINAPI *PFN_GETNUMAHIGHESTNODENUMBER)(PULONG HighestNodeNumber);
typedef BOOL (WINAPI *PFN_GETVERSIONEXW)(LPOSVERSIONINFOW lpVersionInformation);
typedef void (WINAPI *PFN_INITIALIZESLISTHEAD)(PSLIST_HEADER ListHead);
typedef PSLIST_ENTRY (WINAPI *PFN_INTERLOCKEDFLUSHSLIST)(PSLIST_HEADER ListHead);
typedef PSLIST_ENTRY (WINAPI *PFN_INTERLOCKEDPOPENTRYSLIST)(PSLIST_HEADER ListHead);
typedef PSLIST_ENTRY (WINAPI *PFN_INTERLOCKEDPUSHENTRYSLIST)(PSLIST_HEADER ListHead, PSLIST_ENTRY ListEntry);
typedef USHORT (WINAPI *PFN_QUERYDEPTHSLIST)(PSLIST_HEADER ListHead);

static PFN_GETMODULEHANDLEEXW pfnGetModuleHandleExW = nullptr;
static PFN_GETNUMAHIGHESTNODENUMBER pfnGetNumaHighestNodeNumber = nullptr;
static PFN_GETVERSIONEXW pfnGetVersionExW = nullptr;
static PFN_INITIALIZESLISTHEAD pfnInitializeSListHead = nullptr;
static PFN_INTERLOCKEDFLUSHSLIST pfnInterlockedFlushSList = nullptr;
static PFN_INTERLOCKEDPOPENTRYSLIST pfnInterlockedPopEntrySList = nullptr;
static PFN_INTERLOCKEDPUSHENTRYSLIST pfnInterlockedPushEntrySList = nullptr;
static PFN_QUERYDEPTHSLIST pfnQueryDepthSList = nullptr;

static BOOL WINAPI
_DummyGetModuleHandleExW(DWORD dwFlags, LPCWSTR lpModuleName, HMODULE* phModule)
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
_DummyGetNumaHighestNodeNumber(PULONG HighestNodeNumber)
{
    // An operating system so old that it needs this dummy implementation doesn't support NUMA either.
    // So we can correctly return a single NUMA node 0 here.
    *HighestNodeNumber = 0;
    return TRUE;
}

// Verified to functionally match the ReactOS implementation on i386.
static void WINAPI
_DummyInitializeSListHead(PSLIST_HEADER ListHead)
{
    // Due to the union, this member encompasses the entire SLIST_HEADER structure.
    ListHead->Alignment = 0;
}

// Verified to functionally match the ReactOS implementation on i386.
static PSLIST_ENTRY WINAPI
_DummyInterlockedFlushSList(PSLIST_HEADER ListHead)
{
    SLIST_HEADER InitialHead = *ListHead;

    for (;;)
    {
        // Check if the list is already empty.
        if (InitialHead.Next.Next == nullptr)
        {
            // Nothing to do for us and no list entry to return.
            return nullptr;
        }

        // Zero the depth and first list entry pointer while keeping the sequence number intact.
        SLIST_HEADER NewHead = InitialHead;
        NewHead.Depth = 0;
        NewHead.Next.Next = nullptr;

        // Do the following things in a single atomic operation:
        // * Check if the list is still the same we last saved in InitialHead.
        // * If that is the case, update it to our modified NewHead.
        // * If not, someone else was faster than us. Store the changed list in PreviousHead.
        SLIST_HEADER PreviousHead;
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
static PSLIST_ENTRY WINAPI
_DummyInterlockedPopEntrySList(PSLIST_HEADER ListHead)
{
    SLIST_HEADER InitialHead = *ListHead;

    for (;;)
    {
        // Check if the list is empty.
        if (InitialHead.Next.Next == nullptr)
        {
            // Nothing to do for us and no list entry to return.
            return nullptr;
        }

        // Adjust the depth.
        SLIST_HEADER NewHead = InitialHead;
        NewHead.Depth--;

        // Unmount the first list entry.
        // It may have already been freed in the meantime, so check this inside a SEH block.
        __try
        {
            NewHead.Next = *NewHead.Next.Next;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            InitialHead = *ListHead;
            continue;
        }

        // Do the following things in a single atomic operation:
        // * Check if the list is still the same we last saved in InitialHead.
        // * If that is the case, update it to our modified NewHead.
        // * If not, someone else was faster than us. Store the changed list in PreviousHead.
        SLIST_HEADER PreviousHead;
        PreviousHead.Alignment = static_cast<ULONGLONG>(_InterlockedCompareExchange64(
            reinterpret_cast<long long*>(&ListHead->Alignment),
            static_cast<long long>(NewHead.Alignment),
            static_cast<long long>(InitialHead.Alignment)
        ));

        if (InitialHead.Alignment == PreviousHead.Alignment)
        {
            // We successfully popped from the list.
            // Return that list entry.
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
static PSLIST_ENTRY WINAPI
_DummyInterlockedPushEntrySList(PSLIST_HEADER ListHead, PSLIST_ENTRY ListEntry)
{
    SLIST_HEADER InitialHead = *ListHead;

    for (;;)
    {
        // Mount the list entry at the very beginning and adjust depth and sequence.
        SLIST_HEADER NewHead = InitialHead;
        ListEntry->Next = NewHead.Next.Next;
        NewHead.Next.Next = ListEntry;
        NewHead.Depth++;
        NewHead.Sequence++;

        // Do the following things in a single atomic operation:
        // * Check if the list is still the same we last saved in InitialHead.
        // * If that is the case, update it to our modified NewHead.
        // * If not, someone else was faster than us. Store the changed list in PreviousHead.
        SLIST_HEADER PreviousHead;
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

// Verified to functionally match the ReactOS implementation on i386.
static USHORT WINAPI
_DummyQueryDepthSList(PSLIST_HEADER ListHead)
{
    return ListHead->Depth;
}

extern "C" BOOL WINAPI
_imp__GetModuleHandleExW(DWORD dwFlags, LPCWSTR lpModuleName, HMODULE * phModule)
{
    if (!pfnGetModuleHandleExW)
    {
        // Check if the API is provided by kernel32, otherwise fall back to our dummy implementation.
        HMODULE hKernel32 = GetModuleHandleW(L"kernel32");
        pfnGetModuleHandleExW = reinterpret_cast<PFN_GETMODULEHANDLEEXW>(GetProcAddress(hKernel32, "GetModuleHandleExW"));
        if (!pfnGetModuleHandleExW)
        {
            pfnGetModuleHandleExW = _DummyGetModuleHandleExW;
        }
    }

    return pfnGetModuleHandleExW(dwFlags, lpModuleName, phModule);
}

extern "C" BOOL WINAPI
_imp__GetNumaHighestNodeNumber(PULONG HighestNodeNumber)
{
    if (!pfnGetNumaHighestNodeNumber)
    {
        // Check if the API is provided by kernel32, otherwise fall back to our dummy implementation.
        HMODULE hKernel32 = GetModuleHandleW(L"kernel32");
        pfnGetNumaHighestNodeNumber = reinterpret_cast<PFN_GETNUMAHIGHESTNODENUMBER>(GetProcAddress(hKernel32, "GetNumaHighestNodeNumber"));
        if (!pfnGetNumaHighestNodeNumber)
        {
            pfnGetNumaHighestNodeNumber = _DummyGetNumaHighestNodeNumber;
        }
    }

    return pfnGetNumaHighestNodeNumber(HighestNodeNumber);
}

extern "C" BOOL WINAPI
_imp__GetVersionExW(LPOSVERSIONINFOW lpVersionInformation)
{
    if (!pfnGetVersionExW)
    {
        // This API is guaranteed to exist.
        HMODULE hKernel32 = GetModuleHandleW(L"kernel32");
        pfnGetVersionExW = reinterpret_cast<PFN_GETVERSIONEXW>(GetProcAddress(hKernel32, "GetVersionExW"));
    }

    if (!pfnGetVersionExW(lpVersionInformation))
    {
        return FALSE;
    }

    // Check if we are running on Windows 2000.
    if (lpVersionInformation->dwMajorVersion == 5 && lpVersionInformation->dwMinorVersion == 0)
    {
        // Pretend to be Windows XP, which is the minimum version officially supported by the CRT.
        // If we don't do that, the CRT throws ::Concurrency::unsupported_os() in ResourceManager::RetrieveSystemVersionInformation.
        // Fortunately, this is the only function calling GetVersionExW.
        lpVersionInformation->dwMinorVersion = 1;
    }

    return TRUE;
}

extern "C" void WINAPI
_imp__InitializeSListHead(PSLIST_HEADER ListHead)
{
    if (!pfnInitializeSListHead)
    {
        // Check if the API is provided by kernel32, otherwise fall back to our dummy implementation.
        HMODULE hKernel32 = GetModuleHandleW(L"kernel32");
        pfnInitializeSListHead = reinterpret_cast<PFN_INITIALIZESLISTHEAD>(GetProcAddress(hKernel32, "InitializeSListHead"));
        if (!pfnInitializeSListHead)
        {
            pfnInitializeSListHead = _DummyInitializeSListHead;
        }
    }

    return pfnInitializeSListHead(ListHead);
}

extern "C" PSLIST_ENTRY WINAPI
_imp__InterlockedFlushSList(PSLIST_HEADER ListHead)
{
    if (!pfnInterlockedFlushSList)
    {
        // Check if the API is provided by kernel32, otherwise fall back to our dummy implementation.
        HMODULE hKernel32 = GetModuleHandleW(L"kernel32");
        pfnInterlockedFlushSList = reinterpret_cast<PFN_INTERLOCKEDFLUSHSLIST>(GetProcAddress(hKernel32, "InterlockedFlushSList"));
        if (!pfnInterlockedFlushSList)
        {
            pfnInterlockedFlushSList = _DummyInterlockedFlushSList;
        }
    }

    return pfnInterlockedFlushSList(ListHead);
}

extern "C" PSLIST_ENTRY WINAPI
_imp__InterlockedPopEntrySList(PSLIST_HEADER ListHead)
{
    if (!pfnInterlockedPopEntrySList)
    {
        // Check if the API is provided by kernel32, otherwise fall back to our dummy implementation.
        HMODULE hKernel32 = GetModuleHandleW(L"kernel32");
        pfnInterlockedPopEntrySList = reinterpret_cast<PFN_INTERLOCKEDPOPENTRYSLIST>(GetProcAddress(hKernel32, "InterlockedPopEntrySList"));
        if (!pfnInterlockedPopEntrySList)
        {
            pfnInterlockedPopEntrySList = _DummyInterlockedPopEntrySList;
        }
    }

    return pfnInterlockedPopEntrySList(ListHead);
}

extern "C" PSLIST_ENTRY WINAPI
_imp__InterlockedPushEntrySList(PSLIST_HEADER ListHead, PSLIST_ENTRY ListEntry)
{
    if (!pfnInterlockedPushEntrySList)
    {
        // Check if the API is provided by kernel32, otherwise fall back to our dummy implementation.
        HMODULE hKernel32 = GetModuleHandleW(L"kernel32");
        pfnInterlockedPushEntrySList = reinterpret_cast<PFN_INTERLOCKEDPUSHENTRYSLIST>(GetProcAddress(hKernel32, "InterlockedPushEntrySList"));
        if (!pfnInterlockedPushEntrySList)
        {
            pfnInterlockedPushEntrySList = _DummyInterlockedPushEntrySList;
        }
    }

    return pfnInterlockedPushEntrySList(ListHead, ListEntry);
}

extern "C" USHORT WINAPI
_imp__QueryDepthSList(PSLIST_HEADER ListHead)
{
    if (!pfnQueryDepthSList)
    {
        // Check if the API is provided by kernel32, otherwise fall back to our dummy implementation.
        HMODULE hKernel32 = GetModuleHandleW(L"kernel32");
        pfnQueryDepthSList = reinterpret_cast<PFN_QUERYDEPTHSLIST>(GetProcAddress(hKernel32, "QueryDepthSList"));
        if (!pfnQueryDepthSList)
        {
            pfnQueryDepthSList = _DummyQueryDepthSList;
        }
    }

    return pfnQueryDepthSList(ListHead);
}

#endif
