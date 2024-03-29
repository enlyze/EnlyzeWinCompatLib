;
; EnlyzeWinCompatLib - Let Clang-compiled applications run on older Windows versions
; Copyright (c) 2021 Colin Finck, ENLYZE GmbH <c.finck@enlyze.com>
; SPDX-License-Identifier: MIT
;

.model flat

EXTERN _LibGetFileSizeEx@8 : PROC
EXTERN _LibInitializeCriticalSectionAndSpinCount@8 : PROC
EXTERN _LibSetFilePointerEx@20 : PROC

.data

PUBLIC __imp__GetFileSizeEx@8
__imp__GetFileSizeEx@8 dd _LibGetFileSizeEx@8

PUBLIC __imp__InitializeCriticalSectionAndSpinCount@8
__imp__InitializeCriticalSectionAndSpinCount@8 dd _LibInitializeCriticalSectionAndSpinCount@8

PUBLIC __imp__SetFilePointerEx@20
__imp__SetFilePointerEx@20 dd _LibSetFilePointerEx@20

END
