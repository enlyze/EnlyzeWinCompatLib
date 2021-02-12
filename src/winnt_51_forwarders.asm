;
; EnlyzeWinCompatLib - Let MSVC v141_xp targeted applications run on even older Windows versions
; Copyright (c) 2021 Colin Finck, ENLYZE GmbH <c.finck@enlyze.com>
; SPDX-License-Identifier: MIT
;

.model flat

EXTERN _LibDecodePointer@4 : PROC
EXTERN _LibEncodePointer@4 : PROC

.data

PUBLIC __imp__DecodePointer@4
__imp__DecodePointer@4 dd _LibDecodePointer@4

PUBLIC __imp__EncodePointer@4
__imp__EncodePointer@4 dd _LibEncodePointer@4

END
