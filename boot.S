#define KERNEL_VIRT_BASE 0xFFFFFFFF80000000
#define V2P(x)	((x)-(KERNEL_VIRT_BASE))
#define PTE_P   (1 << 0)
#define PTE_W   (1 << 1)
#define PTE_U   (1 << 2)
#define PTE_PS  (1 << 7)

.section .text

.global _start
.extern kernel_main
_start:
  cli
.code32
  mov $0x41, %al
  mov $0x3f8, %dx
  outb %al, %dx
  mov %eax, %edi
  mov %ebx, %esi
  lgdt (V2P(gdt_descriptor))
  mov $0x10, %ax
  mov %ax, %ds
  mov %ax, %es
  mov %ax, %fs
  mov %ax, %gs
  mov %ax, %ss
  ljmpl $0x08, $V2P(flush_cs)
flush_cs:
  mov $V2P(pml4), %ecx
  movl %ecx, %cr3
  movl %cr4, %eax
  # orl  $0x00000010, %eax
	 orl  $(1 << 5), %eax
  movl %eax, %cr4
	 movl $0xC0000080, %ecx
	 rdmsr
	 orl $(1 << 8), %eax
	 wrmsr
	 lgdt (V2P(gdt_descriptor))
	 mov $0x10, %ax
	 mov %ax, %ds
	 mov %ax, %es
	 mov %ax, %fs
	 mov %ax, %gs
	 mov %ax, %ss
	 movl %cr0, %eax
  orl  $0x80000000, %eax
  movl %eax, %cr0
	 ljmp $0x18, $(V2P(higher_half))
.code64
higher_half:
	 mov $0x20, %ax
	 mov %ax, %ds
	 mov %ax, %es
	 mov %ax, %fs
	 mov %ax, %gs
	 mov %ax, %ss
  mov %cr3, %rax
  mov %rax, %cr3
  movabs $stack_top, %rsp
	 movabs $jump_here, %rax
	 jmp *%rax
jump_here:
  mov %rsi, %rsi
	 mov %rdi, %rdi
	 call kernel_main
hang:
  hlt
  jmp hang
.section .data
.align 8
gdt_start:
  .quad 0x0000000000000000  # 0x00
	 .quad 0x00CF9A000000FFFF	# 0x08
	 .quad 0x00CF92000000FFFF	# 0x10
	 .quad 0x00AF9A000000FFFF	# 0x18
	 .quad 0x00CF92000000FFFF	# 0x20
gdt_end:
gdt_descriptor:
  .word gdt_end - gdt_start - 1
  .quad V2P(gdt_start)

.align 4096
boot_page_dir:
  .fill 1024, 4, 0
.align 4096
pml4:
  .quad V2P(pdpt) + 0x03
  .fill 510, 8, 0
  .quad V2P(pdpt) + 0x03
.align 4096
pdpt:
  .quad V2P(pd) + 0x03
  .fill 508, 8, 0
  .quad 0x0000000100000000
  .quad V2P(pd) + 0x03
  .quad 0
.align 4096
pd:
  .set i, 0
  .rept 512
  .quad (i << 21) | 0x83
  .set i, i + 1
  .endr
.align 4096
.section .bss
.align 16
stack_bottom:
    .skip 65536
stack_top:
