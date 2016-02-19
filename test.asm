format PE console
entry start
include 'win32a.inc'

section '.code' code readable writable executable
start:

    var1 dd ?
    v56 dd 0
    mov [v56], eax
    var2 dd 0
    mov [var2], eax
    mov  eax, 0
    va3 dd 0
    mov [va3], eax
test2:
    push ebp
    mov  ebp, esp
    jmp  decl_test2_end
decl_test2_start:
decl_test2_end:
    mov  eax, 123
    push eax
    mov  eax, 5
    push eax
    mov  eax, 70
    pop  ecx
    imul ecx, eax
    mov  eax, ecx
    pop  ecx
    add  ecx, eax
    mov  eax, ecx
    mov  esp, ebp
    pop  ebp
    ret
    mov  esp, ebp
    pop  ebp
    ret
main1:
    push ebp
    mov  ebp, esp
    jmp  decl_main1_end
decl_main1_start:
    i dd ?
    mov  eax, 0
    p dd 0
    mov [p], eax
    mov  eax, 'c'
    v dd 0
    mov [v], eax
    pp dd ?
decl_main1_end:
    mov  eax, 1000
    push eax
    call dword [malloc]
    mov  [pp], eax
    mov  eax, 9
    mov  [i], eax
    mov  eax, [i]
    push eax
    mov  eax, 10
    pop  ecx
    add  ecx, eax
    mov  eax, ecx
    mov  [p], eax
    call dword [test2]
    mov  eax, [pp]
    push eax
    call dword [free]
    mov  eax, 0
    push eax
    call dword [fclose]
    mov  eax, 0
    mov  esp, ebp
    pop  ebp
    ret
    mov  esp, ebp
    pop  ebp
    ret
section '.idata' import data readable
    library msvcr, 'msvcr120d.dll',\
            kernel32, 'kernel32.dll'
    import msvcr, \
           malloc, 'malloc',\
           free, 'free',\
           fclose, 'fclose'