OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)
KERNEL_VIRT_BASE = 0xFFFFFFFF80000000;
KERNEL_PHYS_BASE = 0x00100000;
SECTIONS {
	. = KERNEL_PHYS_BASE + KERNEL_VIRT_BASE;
	kernel_start = .;
	.text ALIGN(4K) : AT(ADDR(.text) - KERNEL_VIRT_BASE) {
		*(.multiboot)
		*(.text)
		*(.text.*)
	}
	.rodata ALIGN(4K) : AT(ADDR(.rodata) - KERNEL_VIRT_BASE) {
		*(.rodata)
		*(.rodata.*)
	}
	.data ALIGN(4K) : AT(ADDR(.data) - KERNEL_VIRT_BASE) {
		*(.data)
		*(.data.*)
	}
	.bss ALIGN(4K) : AT(ADDR(.bss) - KERNEL_VIRT_BASE) {
		__bss_start = .;
		*(COMMON)
		*(.bss)
		*(.bss.*)
		__bss_end = .;
	}
	.percpu : ALIGN(64) {
    __percpu_start = .;
    *(.percpu)
    __percpu_end = .;
	}
	kernel_end = .;
	/DISCARD/ : {
		*(.comment)
		*(.note.*)
		*(.eh_frame)
	}
}
