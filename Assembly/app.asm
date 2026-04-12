IDEAL
MODEL small
STACK 100h

DATASEG
    num db 3          ; המספר שנרצה לחשב לו עצרת
    result db ?       ; כאן תישמר התוצאה

CODESEG
start:
    mov ax, @data
    mov ds, ax

    ; הכנת הפרמטר ב-AL וקריאה לפונקציה הרקורסיבית
    mov al, [num]
    call Factorial    ; התוצאה תחזור ב-AL

    ; שמירת התוצאה והדפסה (בהנחה שהיא חד ספרתית)
    mov [result], al
    mov dl, al
    add dl, '0'
    mov ah, 02h
    int 21h

exit:
    mov ax, 4c00h
    int 21h

; --- פונקציה רקורסיבית לחישוב עצרת ---
proc Factorial
    cmp al, 1         ; תנאי עצירה: אם הגענו ל-1
    jbe stop_rec      ; קפוץ לסיום הרקורסיה

    push ax           ; שומרים את הערך הנוכחי (למשל 3) במחסנית
    dec al            ; מורידים 1 (עכשיו AL הוא 2)
    call Factorial    ; קריאה רקורסיבית: Factorial(n-1)
    
    ; כשחוזרים מהקריאה, התוצאה של (n-1)! נמצאת ב-AL
    pop bx            ; מוציאים מהמחסנית את הערך המקורי ששמרנו (למשל 3) לתוך BL
    mul bl            ; מכפילים: AL = AL * BL
    ret

stop_rec:
    mov al, 1         ; הבסיס: !1 = 1
    ret
endp Factorial

END start