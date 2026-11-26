.code16
.intel_syntax noprefix
.globl _start

.equ KERN_VA, 0xFFFFFFFFC0000000    # must match kernel.ld
.equ HI32,    0xFFFFFFFF00000000    # (sym - HI32) = 0xC000xxxx, a 32-bit value

.section .text
_start:
    cli
    xor ax, ax
    mov es, ax
    mov ds, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov sp, 0x8000
    mov al, 'A'
    mov dx, 0x3f8
    out dx, al

    mov eax, offset gdt_ptr - KERN_VA
    lgdt [eax]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    # far jmp ptr16:32 in a 16-bit section: 66 EA <off32> <sel16>
    .byte 0x66, 0xEA
    .long pmode - KERN_VA
    .word 0x08

.code32
pmode:
    mov ax, 0x10
    mov es, ax
    mov ds, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x8000                 # low stack, paging still off
    mov al, 'P'
    mov dx, 0x3f8
    out dx, al

    mov eax, offset pd - KERN_VA
    mov cr3, eax
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax                    # EIP is low; identity map keeps it alive
    mov eax, offset higher - HI32   # 32-bit absolute high address
    jmp eax

higher:
    mov esp, 0xC000F000             # high stack (phys 0xF000)
    call kernel_main                # both high now, so a relative call works
1:  hlt
    jmp 1b

.section .data
.balign 8
gdt_start:
    .quad 0
    .quad 0x00CF9A000000FFFF        # 0x08: 32-bit code
    .quad 0x00CF92000000FFFF        # 0x10: 32-bit data
    .quad 0x00AF9A000000FFFF        # 0x18: (64-bit code, unused for now)
gdt_end:
gdt_ptr:
    .word gdt_end - gdt_start - 1
    .long gdt_start - KERN_VA

.balign 4096
pd:
    .long (pt - KERN_VA) + 3        # index 0: identity, first 4 MB
    .fill 767, 4, 0                 # indices 1..767
    .long (pt - KERN_VA) + 3        # index 768: 0xC0000000
    .fill 255, 4, 0                 # indices 769..1023

.balign 4096
pt:
    .long 0                         # page 0 unmapped (null guard)
    .set i, 1
    .rept 1023
    .long (i << 12) + 3
    .set i, i + 1
    .endr
