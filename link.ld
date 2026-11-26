OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

KERN_VA = 0xFFFFFFFFC0000000;
KERN_PA = 0x1000;

SECTIONS {
    . = KERN_VA + KERN_PA;
    .text   : AT(ADDR(.text)   - KERN_VA) { *(.text*) }
    .rodata : AT(ADDR(.rodata) - KERN_VA) { *(.rodata*) }
    .data   : AT(ADDR(.data)   - KERN_VA) { *(.data*) }
    .bss    : AT(ADDR(.bss)    - KERN_VA) { *(.bss*) *(COMMON) }
    /DISCARD/ : { *(.note*) *(.comment) *(.eh_frame*) }
}
