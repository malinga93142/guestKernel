#ifndef IDT_HPP
#define IDT_HPP
#include <cstdint>
#include "string.hpp"
#include "exceptions.hpp"

 extern "C" void *isr_stub_table[32];
namespace InterruptDescTable {
 struct Idt64 {
  uint16_t base_low; // 2 bytes
  uint16_t selector; // 2 bytes
  uint8_t ist;       // 1 byte
  uint8_t type_attr; // 1 bytes
  uint16_t base_mid; // 2 bytes
  uint32_t base_high;// 4 bytes
  uint32_t zero;     // 4 byte
 } __attribute__((packed));
 static_assert(sizeof(struct Idt64) == 16, "size mismatch of Idt64");
 struct Idt64Ptr {
  uint16_t limit;
  uint64_t base;
 } __attribute__((packed));
 static_assert(sizeof(struct Idt64Ptr) == 10, "size mismatch of Idt64Ptr");
 extern struct Idt64 idts[256];
 extern struct Idt64Ptr idtPtr;
 inline void setupEntry(Idt64 *idt, uint16_t selector, uint64_t base, uint64_t type_attr) {
  idt->base_low = (base & 0xFFFF);
  idt->base_mid = ((base >> 0x10) & 0xFFFF);
  idt->base_high= ((base >> 0x20) & 0xFFFFFFFF);
  idt->selector = selector;
  idt->type_attr= type_attr;
  idt->zero     = 0;
  idt->ist      = 0;
 }
 inline void lidt64(const struct Idt64Ptr *__idtptr) {
  __asm__ volatile("lidt (%0)"::"r"(__idtptr):"memory");
 }
 void initIdt64(void);
}
#endif