;
; EnlyzeWinCompatLib - Let MSVC v141_xp targeted applications run on even older Windows versions
; Copyright (c) 2021 Colin Finck, ENLYZE GmbH <c.finck@enlyze.com>
; SPDX-License-Identifier: MIT
;

.model flat

EXTERN _LibGetModuleHandleExW@12 : PROC
EXTERN _LibGetNumaHighestNodeNumber@4 : PROC
EXTERN _LibGetVersionExW@4 : PROC
EXTERN _LibInitializeSListHead@4 : PROC
EXTERN _LibInterlockedFlushSList@4 : PROC
EXTERN _LibInterlockedPopEntrySList@4 : PROC
EXTERN _LibInterlockedPushEntrySList@8 : PROC
EXTERN _LibQueryDepthSList@4 : PROC

.data

PUBLIC __imp__GetModuleHandleExW@12
__imp__GetModuleHandleExW@12 dd _LibGetModuleHandleExW@12

PUBLIC __imp__GetNumaHighestNodeNumber@4
__imp__GetNumaHighestNodeNumber@4 dd _LibGetNumaHighestNodeNumber@4

PUBLIC __imp__GetVersionExW@4
__imp__GetVersionExW@4 dd _LibGetVersionExW@4

PUBLIC __imp__InitializeSListHead@4
__imp__InitializeSListHead@4 dd _LibInitializeSListHead@4

PUBLIC __imp__InterlockedFlushSList@4
__imp__InterlockedFlushSList@4 dd _LibInterlockedFlushSList@4

PUBLIC __imp__InterlockedPopEntrySList@4
__imp__InterlockedPopEntrySList@4 dd _LibInterlockedPopEntrySList@4

PUBLIC __imp__InterlockedPushEntrySList@8
__imp__InterlockedPushEntrySList@8 dd _LibInterlockedPushEntrySList@8

PUBLIC __imp__QueryDepthSList@4
__imp__QueryDepthSList@4 dd _LibQueryDepthSList@4

END
