#ifndef PRINT_HPP
#define PRINT_HPP
#include <cstdint>
#define SERIAL_PORT 0x3F8
namespace print {
 static inline void outb(uint16_t port, uint8_t val) {
  __asm__ volatile("outb %0, %1" ::"a"(val), "Nd"(port));
 }
 static void println(const char* p) {
  while (*p) {
   outb(SERIAL_PORT, *p);
   p++;
  }
 }
}
#endif
