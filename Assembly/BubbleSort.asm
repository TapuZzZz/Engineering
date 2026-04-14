IDEAL
MODEL small
STACK 100h

DATASEG
   arr dw 5, 2, 8, 1, 9
   size dw 5

CODESEG
start:
    mov ax, @data
    mov ds, ax

    mov cx, [size]

first_loop:
    push cx
    lea si, [arr]

second_loop:
    mov ax, [si]
    cmp ax, [si+2]
    jng skip_swap

    push si
    lea dx, [si+2]
    push dx
    call swap

skip_swap:
    add si, 2
    loop second_loop

    pop cx
    loop first_loop

exit:
    mov ax, 4c00h
    int 21h
    

proc swap
    push bp
    mov bp, sp

    push ax
    push bx
    push si
    push di

    mov si, [bp+6]
    mov di, [bp+4]
    
    mov ax, [si]
    mov bx, [di]
    mov [si], bx
    mov [di], ax

    pop di
    pop si
    pop bx
    pop ax

    pop bp
    ret 4
endp swap

END start