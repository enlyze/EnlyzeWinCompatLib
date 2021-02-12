;
; EnlyzeWinCompatLib - Let MSVC v141_xp targeted applications run on even older Windows versions
; Copyright (c) 2021 Colin Finck, ENLYZE GmbH <c.finck@enlyze.com>
; SPDX-License-Identifier: MIT
;

.model flat

EXTERN _LibGetLogicalProcessorInformation@8 : PROC

.data

PUBLIC __imp__GetLogicalProcessorInformation@8
__imp__GetLogicalProcessorInformation@8 dd _LibGetLogicalProcessorInformation@8

END
