IDEAL
MODEL small
STACK 100h

DATASEG
    arr dw 500, 300, 522, 133, 524, -1
    cnt db 0 

CODESEG
start:
    mov ax, @data
    mov ds, ax

    lea si, [arr]
next_item:
    mov ax, [si]
    cmp ax, -1
    je exit

    test ax, 1
    jnz skip_next
    inc [cnt]

skip_next:
    add si, 2
    jmp next_item


exit:

    mov dl, [cnt]
    add dl, '0'
    mov ah, 02h
    int 21h
    mov ax, 4c00h
    int 21h
    
END start