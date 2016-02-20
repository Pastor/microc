format PE GUI
entry start

section '.data' data readable writable
base_address  dd  0
section '.text' code readable executable writable
;base_address  dd  0
start:
  call	  $ + 5
  pop	  eax	       ; base_address
  sub	  eax, 5
  mov	  [base_address], eax
  push	  ebx
  mov	  ebx, [base_address]
  add	  ebx, (sub_0000 - start)
  push	  4
  call	  ebx
  pop	  ebx
  ret

sub_0000:
  push ebp
  mov  ebp, esp
  mov  eax, [ebp + 0x08]
  add  eax, 0x04
  push ebx
  mov  ebx, [base_address]
  add  ebx, (sub_0001 - start)
  push eax
  call ebx
  pop  ebx
  pop  ebp
  ret  0x04

sub_0001:
  push ebp
  mov  ebp, esp
  mov  eax, [ebp + 0x08]
  add  eax, 0x04
  pop  ebp
  ret  0x04
