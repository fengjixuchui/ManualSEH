;
; ManualSEH.asm
;
;	Shellcode for ManualSEH exception handling
;

.code

__MSEH_ENTER_TRY PROC
	mov rax, 0deadbeef000005e1h
	int 3
	ret
__MSEH_ENTER_TRY ENDP

__MSEH_EXIT_TRY PROC
	mov rax, 0deadbeef000005e2h
	int 3
	ret
__MSEH_EXIT_TRY ENDP

END