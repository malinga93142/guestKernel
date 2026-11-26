#include <sys/types.h>
#include <cstdint>
#include "gdt.hpp"
#include "idt.hpp"
#include "pmm.hpp"
#include "print.hpp"

#define HALT() __asm__ volatile("hlt")
void write_here(uint8_t val) {
 volatile uint8_t *a = (volatile uint8_t*)0x500000;
 *a = val;
}
extern "C" [[noreturn]] void kernel_main(void) {
  print::outb(SERIAL_PORT, 0xa);
  print::outb(SERIAL_PORT, 'A');
  print::outb(SERIAL_PORT, 'B');
  print::outb(SERIAL_PORT, 'C');
  print::outb(SERIAL_PORT, 0xa);
  print::println("YC\n");
  print::println("Malinga RK");
  print::println("\n");
  pmm::pmm_init();
  GlobalDescTable::setupGdt64();
  InterruptDescTable::initIdt64();
  write_here(0xa);
  print::outb(0x501, 1);
  while (1) HALT();
}
