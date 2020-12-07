;
; EnlyzeWinCompatLib - Let MSVC v141_xp targeted applications run on even older Windows versions
; Written by Colin Finck for ENLYZE GmbH
;

.model flat

EXTERN _LibInitializeCriticalSectionAndSpinCount@8 : PROC
EXTERN _LibSetFilePointerEx@20 : PROC

.data

PUBLIC __imp__InitializeCriticalSectionAndSpinCount@8
__imp__InitializeCriticalSectionAndSpinCount@8 dd _LibInitializeCriticalSectionAndSpinCount@8

PUBLIC __imp__SetFilePointerEx@20
__imp__SetFilePointerEx@20 dd _LibSetFilePointerEx@20

END
