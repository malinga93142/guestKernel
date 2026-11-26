#pragma once
#include <cstdint>
namespace GlobalDescTable {
struct Gdt {
  uint16_t limit_low;
  uint16_t base_low;
  uint8_t base_mid;
  uint8_t access;
  uint8_t granularity;
  uint8_t base_high;
} __attribute__((packed));

struct GdtPtr {
  uint16_t limit;
  uint32_t base;
} __attribute__((packed));
// _Static_assert(sizeof(struct GdtPtr) == 0x8, "size mismatch");
static Gdt gdt[4] = {{0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00},
                     {0xFFFF, 0x0000, 0x00, 0x9A, 0xCF, 0x00},
                     {0xFFFF, 0x0000, 0x00, 0x92, 0xCF, 0x00},
                     {0xFFFF, 0x0000, 0x00, 0xFA, 0xCF, 0x00}};
static inline void lgdt(GdtPtr* __ptr) {
  __asm__ volatile("lgdt (%0)" ::"r"(__ptr) : "memory");
}

inline void setupGdt(void) {
  GdtPtr ptr;
  ptr.limit = sizeof(gdt) - 1;
  ptr.base = (uint32_t)(uintptr_t)gdt;
  // lgdt(&ptr);
  __asm__ volatile("lgdt (%0)" ::"r"(&ptr) : "memory");
  __asm__ volatile(
      "movw $0x10, %%ax\n\t"
      "movw %%ax, %%ss\n\t"
      "movw %%ax, %%ds\n\t"
      "movw %%ax, %%es\n\t"
      "movw %%ax, %%fs\n\t"
      "movw %%ax, %%gs\n\t"
      "push $0x08\n\t"
      "push $1f\n\t"
      "lretl\n\t"
      "1:\n\t" ::
          : "ax", "memory", "cc");
}

// 64 bit mode
struct Gdt64 {
  uint16_t limit_low;
  uint16_t base_low;
  uint8_t base_mid;
  uint8_t access;
  uint8_t granularity;
  uint8_t base_high;
} __attribute__((packed));
static_assert(sizeof(Gdt64) == 8, "size mismatch of gdt64");

struct GdtPtr64 {
 uint16_t limit;
 uint64_t base;
} __attribute__((packed));
static_assert(sizeof(GdtPtr64) == 10, "size mismatch of gdtptr64");

static Gdt64 gdt64[4] = {
    {0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00}, // 0x00: Null Descriptor
    {0x0000, 0x0000, 0x00, 0x9A, 0xAF, 0x00}, // 0x08: Kernel Code (64-bit, L=1, Granularity 0xAF)
    {0x0000, 0x0000, 0x00, 0x92, 0xCF, 0x00}, // 0x10: Kernel Data (32/64-bit data, Granularity 0xCF)
    {0x0000, 0x0000, 0x00, 0xFA, 0xAF, 0x00}  // 0x18: User Code (64-bit Ring 3, L=1, Granularity 0xAF)
};

static inline void lgdt64(GdtPtr64* __ptr) {
  __asm__ volatile("lgdt (%0)" ::"r"(__ptr) : "memory");
}

static GdtPtr64 ptr64;
inline void setupGdt64(void) {
  ptr64.limit = sizeof(gdt64) - 1;
  ptr64.base = reinterpret_cast<uintptr_t>(&gdt64);
  lgdt64(&ptr64);
  asm volatile("mov $0x10, %%eax\n\t"
               "mov %%eax, %%ss\n\t"
               "mov %%eax, %%ds\n\t"
               "mov %%eax, %%es\n\t"
               "mov %%eax, %%fs\n\t"
               "mov %%eax, %%gs" ::: "rax");
  // far jmp or long jmp? whatever, it works.
  asm volatile("pushq $0x08\n\t"
               "leaq 1f(%%rip), %%rax\n\t"
               "pushq %%rax\n\t"
               "lretq\n\t"
               "1:" ::: "rax", "memory");
}
} // GlobalDescTab;e