;       COMMINT.ASM (C)1993 NPS Software
;
;
;       Written By: Brad Broerman
;       Last Modified: 3/13/97
;       Version 2.0
;
;   This is the interrupt handler for the commport. Compile 
; it with the following: "TASM /m2 /la COMMINT.ASM" and link
; it with COMMPORT.CPP in the Turbo C++ IDE. Make sure to print
; a FULL MAP. This map file output is what is to be added into the
; c++ program.
;
.386
DGROUP group _DATA,_BSS
        assume cs:COMMINT_TEXT,ds:DGROUP

_BSS segment word public 'BSS'
_BSS ends

_DATA segment word public 'BSS'
extrn _COMM_PortBase:word
extrn _COMM_PortINT:byte
extrn _COMM_Buffer:dword
extrn _COMM_Buffer_Tail:word
extrn _COMM_Buffer_MaxSize:word
extrn _COMM_Buffer_Size:word
extrn _COMM_Buffer_Head:word
_DATA ends

public _CommInterruptHndlr

COMMINT_TEXT segment byte public "CODE"
_CommInterruptHndlr proc far
        push ds                         ; Store the flags and registers.
	pusha
        pushf
        mov ax,DGROUP                   ; Move the C++ Data Segment into DS.
        mov ds,ax
;
;  Here's where we'll get ready to handle the interrupt. We'll turn off the 
; UART's interrupt, and then rts. 
;

        mov dx,_COMM_PortBase           ; Point to the Interrupt Enable register,
        inc dx
        xor ax,ax
        out dx,al                       ; and turn off interrupts from this ISR.
	mov dx, _COMM_PortBase		; Point to the modem control register
	add dx,4
	in al, dx
	and al,11111101b
	out dx,al			; and turn off RTS.
;
;  Here is the main outer loop. it checks for an interrupt, processes only the
; data available interrupt, and then reads the status registers to clear them
; (to clear any other pending interrupts, just in case).
;

COMM_Start_Int:
	mov dx, _COMM_PortBase		; Now, point to the Interrupt Id Register
	inc dx
	inc dx
	in al,dx
	test al,1			; Test to see if there is an interrupt.
	jnz COMM_ALL_DONE		; If none, then quit.
	and al,06h
	cmp al,04h			; else, is there a receive interrupt?
	je COMM_Get_Byte		; If so, go handle it.
	inc dx				; else, read the status registers, and clear them.
	inc dx
	inc dx
	in al,dx
	inc dx
	in al,dx
	jmp COMM_Start_Int
;
;  Here's where we actually get the information and put it into the buffer.
;

COMM_Get_Byte:
        mov ax,_COMM_Buffer_Size        ; Get the current buffer size,
        inc ax                          ; and increment it.
        cmp ax,_COMM_Buffer_MaxSize     ; Too big?
        jb COMM_CheckTail               ; If not, continue
        mov dx,_COMM_PortBase           ; If so,
        in al,dx                        ; then get the byte, 
        jmp COMM_ALL_DONE               ; and ignore it.

COMM_CheckTail:
        mov _COMM_Buffer_Size,ax        ; Store the new buffer size.
        lea bx,DGROUP:_COMM_Buffer      ; Get the address of the buffer.
        mov di,_COMM_Buffer_Tail        ; Get the offset of the tail,
        inc di                          ; and increment it.
        cmp di,_COMM_Buffer_MaxSize     ; Is it at the end?
        jb COMM_StoreIt                 ; If not, go ahead and store it.
        xor di,di                       ; else, reset it to zero.

COMM_StoreIt:
        mov _COMM_Buffer_Tail,di        ; Store the new tail location.
        mov dx,_COMM_PortBase           ; Get the byte from the RDR,
        in al,dx
	mov al,32			; We're debugging here...
        mov [bx+di],al                  ; and store it in the buffer.
        mov dx,_COMM_PortBase           ; Point to interrupt ID register,
        add dx,5
        in al,dx                        ; and get status.
        test al,1                       ; Is another interrupt pending?
        jnz COMM_Get_Byte               ; If so, go again.
	jmp COMM_Start_Int		;
;
;   Finally, when we're all done, we jump down here and clean up.
;

COMM_ALL_DONE:
        mov dx,_COMM_PortBase           ; Point to the modem control register,
        add dx,4
        in al,dx
        or al,02h                       
        out dx,al                       ; and turn RTS back on, 
	mov dx,_COMM_PortBase		; as well as the UART's interrupts.
	inc dx
	mov al,1
	out dx,al
	mov al, _COMM_PortINT		; Now we tell the appropriate PICs we're
	cmp al,8			; finishing an interrupt...
	jb COMM_JUST_20			; See if we're doing both, or
	mov al,020h
	out 0A0h, al
COM_JUST_20:  				; just the lower one.
	mov al,020h
	out 020h, al
        popf                            ; Pop flags and register.
	popa
        pop ds
        iret                            ; All done, return...
_CommInterruptHndlr endp
COMMINT_TEXT ends
        END

