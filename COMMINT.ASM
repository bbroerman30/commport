;       COMMINT.ASM (C)1993 NPS Software
;
;
;       Written By: Brad Broerman
;       Last Modified: 7/12/95
;       Version 1.0
;
;   This is the interrupt handler for the commport. Compile 
; it with the following: "TASM /m2 /la COMMINT.ASM" and link
; it with COMMPORT.CPP in the Turbo C++ IDE.
;

DGROUP group _DATA,_BSS
        assume cs:COMMINT_TEXT,ds:DGROUP

_BSS segment word public 'BSS'
_BSS ends

_DATA segment word public 'BSS'
extrn _COMM_PortBase:word
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
        push ax
        push bx
        push dx
        push di
        pushf
        mov ax,DGROUP                   ; Move the C++ Data Segment into DS.
        mov ds,ax

        mov dx,_COMM_PortBase           ; Point to the modem control register,
        add dx,4
        in al,dx
        and al,11111101b
        out dx,al                       ; and turn off RTS.

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
        mov [bx+di],al                  ; and store it in the buffer.
        mov dx,_COMM_PortBase           ; Point to interrupt ID register,
        inc dx
        inc dx
        in al,dx                        ; and get status.
        test al,1                       ; Is another interrupt pending?
        jz COMM_Get_Byte                ; If so, go again.

COMM_ALL_DONE:
        mov dx,_COMM_PortBase           ; Point to the modem control register,
        add dx,4
        in al,dx
        or al,02h                       
        out dx,al                       ; and turn RTS back on.
        mov al,20h
        out 20h,al                      ; Signal end of Interrupt.
        popf                            ; Pop flags and register.
        pop di
        pop dx
        pop bx
        pop ax
        pop ds
        iret                            ; All done, return...
_CommInterruptHndlr endp
COMMINT_TEXT ends
        END

