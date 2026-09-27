.globl _start
_start:
    xor %rbp,%rbp
    and $-16,%rsp
    call bare_main
    hlt
