;
; EnlyzeWinCompatLib - Let Clang-compiled applications run on older Windows versions
; Written by Colin Finck for ENLYZE GmbH
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
