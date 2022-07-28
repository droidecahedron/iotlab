; G8RTOS_SchedulerASM.s
; Holds all ASM functions needed for the scheduler
; Note: If you have an h file, do not have a C file and an S file of the same name

	; Functions Defined
	.def G8RTOS_Start, PendSV_Handler

	; Dependencies
	.ref CurrentlyRunningThread, G8RTOS_Scheduler

	.thumb		; Set to thumb mode
	.align 2	; Align by 2 bytes (thumb mode uses allignment by 2 or 4)
	.text		; Text section

; Need to have the address defined in file 
; (label needs to be close enough to asm code to be reached with PC relative addressing)
RunningPtr: .field CurrentlyRunningThread, 32

; G8RTOS_Start
;	Sets the first thread to be the currently running thread
;	Starts the currently running thread by setting Link Register to tcb's Program Counter
G8RTOS_Start:

	.asmfunc
	ldr r0,RunningPtr	;&runningptr->r0
	ldr r0,[r0]		;*runningptr->r0 (threadpointer)
	ldr sp,[r0]		;*threadpointer->stack pointer
	pop{r4-r11}		;pop thread registers
	pop{r0-r3}
	pop{r12}
	add sp,sp,#4	;skip over lr
	pop{lr}			;pop PC into lr
	add sp,sp,#4	;skip PSR
	CPSIE I			;enable interrupts
	bx lr			;return to link register [leaving asm func]

	.endasmfunc

; PendSV_Handler
; - Performs a context switch in G8RTOS
; 	- Saves remaining registers into thread stack
;	- Saves current stack pointer to tcb
;	- Calls G8RTOS_Scheduler to get new tcb
;	- Set stack pointer to new stack pointer from new tcb
;	- Pops registers from thread stack
PendSV_Handler:
	
	.asmfunc
	push {r4-r11}		;push registers to save thread A context
	ldr r4,RunningPtr	;load r4 with pointer to tcb pointer
	ldr r5,[r4]			;load r5 with contents of pointer (tcbpointer)
	str sp,[r5]			;store stackpointer to tcbpointer (stackpointer)
	push {lr}
	bl G8RTOS_Scheduler	;Call scheduler to update current thread
	pop {lr}			;comes back after
	ldr r4,RunningPtr	;load r4 with pointer to tcb pointer	(IMPORTANT TO DO THIS AGAIN)
	ldr r5,[r4]			;load r5 with contents of pointer (tcbpointer)
	ldr sp,[r5]			;load sp with new tcb stack pointer
	pop {r4-r11}		;pop registers to restore new thread context

	bx lr				;return to link register [leasing asm func]

	.endasmfunc
	
	; end of the asm file
	.align
	.end
