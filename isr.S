#include "exceptions.hpp"
.intel_syntax noprefix
.equ REG_SIZE, 8
.equ NUM_GP_REGS, 15
.equ PUSHALL_BYTES, (NUM_GP_REGS * REG_SIZE)
.equ CLEANUP_ERRVEC, 16
.extern handler
.macro PUSHALL
 push rax
 push rbx
 push rcx
 push rdx
 push rsi
 push rdi
 push rbp
 push r8
 push r9
 push r10
 push r11
 push r12
 push r13
 push r14
 push r15
.endm
.macro POPALL
 pop r15
 pop r14
 pop r13
 pop r12
 pop r11
 pop r10
 pop r9
 pop r8
 pop rbp
 pop rdi
 pop rsi
 pop rdx
 pop rcx
 pop rbx
 pop rax
.endm
.macro ISR_ERR num
.globl isr\num
isr\num:
 push \num
 jmp isrCommon
.endm

.macro ISR_NOERR num
.global isr\num
isr\num:
 push 0 # fake error code
 push \num
 jmp isrCommon
.endm
isrCommon:
 PUSHALL
 call handler
 POPALL
 add rsp, CLEANUP_ERRVEC
 iretq


ISR_NOERR  0 // Division Error
ISR_NOERR  1 // Debug
ISR_NOERR  2 // Non-Maskable Interrupt
ISR_NOERR  3 // BreakPoint
ISR_NOERR  4 // Overflow
ISR_NOERR  5 // Bound Range Exceed
ISR_NOERR  6 // Invalid Opcode
ISR_NOERR  7 // Device Not Available (FPU)
ISR_ERR    8 // Double Fault
ISR_NOERR  9 // Coprocessor Segment Overrun
ISR_ERR   10 // Invalid TSS
ISR_ERR   11 // Segment Not Present
ISR_ERR   12 // Stack-Segment Fault
ISR_ERR   13 // General Protection Fault
ISR_ERR   14 // Page Fault
ISR_NOERR 15 // Reserved
ISR_NOERR 16 // x87 Floating-Point Exception
ISR_ERR   17 // Alignment Check
ISR_NOERR 18 // Machine Check
ISR_NOERR 19 // SIMD Floating-Point Exception
ISR_NOERR 20 // Virtualization Exception
ISR_NOERR 21 // Control Protection Exception
ISR_NOERR 22 // Reserved
ISR_NOERR 23 // Reserved
ISR_NOERR 24 // Reserved
ISR_NOERR 25 // Reserved
ISR_NOERR 26 // Reserved
ISR_NOERR 27 // Reserved
ISR_NOERR 28 // Hypervisor Injection Exception
ISR_NOERR 29 // VMM Communication Exception
ISR_ERR   30 // Security Exception
ISR_NOERR 31 // Reserved
.text
.globl isr_stub_table
.align 8
isr_stub_table:
 .quad isr0
 .quad isr1
 .quad isr2
 .quad isr3
 .quad isr4
 .quad isr5
 .quad isr6
 .quad isr7
 .quad isr8
 .quad isr9
 .quad isr10
 .quad isr11
 .quad isr12
 .quad isr13
 .quad isr14
 .quad isr15
 .quad isr16
 .quad isr17
 .quad isr18
 .quad isr19
 .quad isr20
 .quad isr21
 .quad isr22
 .quad isr23
 .quad isr24
 .quad isr25
 .quad isr26
 .quad isr27
 .quad isr28
 .quad isr29
 .quad isr30
 .quad isr31