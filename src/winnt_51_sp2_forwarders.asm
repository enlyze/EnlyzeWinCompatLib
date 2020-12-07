;
; EnlyzeWinCompatLib - Let MSVC v141_xp targeted applications run on even older Windows versions
; Written by Colin Finck for ENLYZE GmbH
;

.model flat

EXTERN _LibGetLogicalProcessorInformation@8 : PROC

.data

PUBLIC __imp__GetLogicalProcessorInformation@8
__imp__GetLogicalProcessorInformation@8 dd _LibGetLogicalProcessorInformation@8

END
