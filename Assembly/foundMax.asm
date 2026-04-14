IDEAL
MODEL small
STACK 100h

DATASEG
    arr dw 500, 300, 522, 133, 524, -1
    max dw ?

CODESEG
start:
    mov ax, @data
    mov ds, ax
    
    lea si, [arr]
    mov ax, [si]
    mov [max], ax

next_item:
    mov ax, [si]
    cmp ax, -1
    jz print_start

    cmp [max], ax
    jg go_next
    mov [max], ax

go_next:
    add si, 2
    jmp next_item

print_start:
    mov ax, [max]
    mov bx, 10
    mov cx, 0

split_loop:
    mov dx, 0
    div bx
    push dx
    inc cx
    cmp ax, 0
    jne split_loop

print_loop:
    pop dx
    add dl, '0'
    mov ah, 02h
    int 21h
    loop print_loop

exit:
    mov ax, 4c00h
    int 21h
    
END start