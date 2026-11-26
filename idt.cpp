#include "idt.hpp"

namespace InterruptDescTable {
 struct Idt64 idts[256];
 struct Idt64Ptr idtPtr;
 void initIdt64(void) {
  for (int i = 0; i < 256; i++) {
   String::memSet(&idts[i], 0, sizeof(idts[i]));
  }
  setupEntry(&idts[0], 0x08, reinterpret_cast<uint64_t>(isr_stub_table[0]), 0x8E);
  idtPtr.base = reinterpret_cast<uint64_t>(&idts);
  idtPtr.limit = sizeof(idts) - 1;
  lidt64(&idtPtr);
 }
}