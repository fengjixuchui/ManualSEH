;
; ManualSEH.asm
;
;	Shellcode for ManualSEH exception handling
;

EXTERN ManualSehPushEntry     : PROC
EXTERN ManualSehCurrentThread : PROC

.code

CaptureContext MACRO
	pushfq

	;
	; Capture volatile registers
	;
	mov     [rsp+080h], rax
	mov     [rsp+088h], rcx
	mov     [rsp+090h], rdx
	mov     [rsp+0C0h], r8
	mov     [rsp+0C8h], r9
	mov     [rsp+0D0h], r10
	mov     [rsp+0D8h], r11
	movaps  xmmword ptr [rsp+1A8h], xmm0
	movaps  xmmword ptr [rsp+1B8h], xmm1
	movaps  xmmword ptr [rsp+1C8h], xmm2
	movaps  xmmword ptr [rsp+1D8h], xmm3
	movaps  xmmword ptr [rsp+1E8h], xmm4
	movaps  xmmword ptr [rsp+1F8h], xmm5
	;

	;
	; Capture non volatile registers
	;
	mov     word ptr [rsp+040h], cs
	mov     word ptr [rsp+042h], ds
	mov     word ptr [rsp+044h], es
	mov     word ptr [rsp+04Ah], ss
	mov     word ptr [rsp+046h], fs
	mov     word ptr [rsp+048h], gs

	push	rbp
	mov	rbp, [rsp+4E0h]
	mov     [rsp+0B0h], rbp
	pop	rbp

	mov     [rsp+098h], rbx
	mov     [rsp+0B0h], rsi
	mov     [rsp+0B8h], rdi
	mov     [rsp+0E0h], r12
	mov     [rsp+0E8h], r13
	mov     [rsp+0F0h], r14
	mov     [rsp+0F8h], r15

	fnstcw  word ptr [rsp+108h]
	mov     dword ptr [rsp+10Ah], 0
	movaps  xmmword ptr [rsp+208h], xmm6
	movaps  xmmword ptr [rsp+218h], xmm7
	movaps  xmmword ptr [rsp+228h], xmm8
	movaps  xmmword ptr [rsp+238h], xmm9
	movaps  xmmword ptr [rsp+248h], xmm10
	movaps  xmmword ptr [rsp+258h], xmm11
	movaps  xmmword ptr [rsp+268h], xmm12
	movaps  xmmword ptr [rsp+278h], xmm13
	movaps  xmmword ptr [rsp+288h], xmm14
	movaps  xmmword ptr [rsp+298h], xmm15
	stmxcsr dword ptr [rsp+120h]
	stmxcsr dword ptr [rsp+03Ch]

	lea     rax, [rsp+4E8h]
	mov     [rsp+0A0h], rax
	mov     rax, [rsp+4E0h]
	mov     [rsp+100h], rax
	mov     eax, [rsp]
	mov     [rsp+04Ch], eax
	mov     dword ptr [rsp+038h], 10000Fh
	add     rsp, 8
	;
ENDM

__MSEH_ENTER_TRY PROC
	push rbp
	mov rbp, rsp

	;
	; Capture current context to the stack frame
	;
	sub rsp, 04D0h
	CaptureContext

	call ManualSehCurrentThread

	mov rcx, rsp
	mov rdx, rax
	xor rax, rax
	call ManualSehPushEntry

	mov rsp, rbp
	pop rbp
	ret
__MSEH_ENTER_TRY ENDP

END
