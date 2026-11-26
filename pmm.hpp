#ifndef PMM_H
#define PMM_H
#include <cstdint>
#include <cstddef>
#include "print.hpp"
#define KBASE 0xFFFFFFFF80000000ULL
#define SIZE (32ULL * 1024 * 1024)
#define PAGE_SIZE 4096
#define TOTAL_FRAMES SIZE/PAGE_SIZE
static uint8_t bitmap[TOTAL_FRAMES];
extern char kernel_start[];
extern char kernel_end[];
namespace pmm{
 inline void init_bitmap(void) {
  for (size_t i = 0; i < TOTAL_FRAMES; i++) {
   bitmap[i] = 0xFF;
  }
 }
 static inline void bitmap_set(uint64_t frame) {
  bitmap[frame >> 3] |= (uint8_t)(1 << (frame & 7));
 }
 static inline void bitmap_clr(uint64_t frame) {
  bitmap[frame >> 3] &= ~(uint8_t)(1 << (frame & 7));
 }
 static inline bool bitmap_test(uint64_t frame) {
  return !!(bitmap[frame >> 3] & (uint8_t)(1 << (frame & 0x7)));
 }
 static inline void reserve_region(uint64_t start, uint64_t len) {
  uint64_t start_frame = (start / PAGE_SIZE);
  uint64_t end_frame   = ((start + len) + (PAGE_SIZE-1)) / PAGE_SIZE;
  if (end_frame > TOTAL_FRAMES) return;
  for (uint64_t i = start_frame; i < end_frame; i += 1) {
   if (!bitmap_test(i)) {
    bitmap_set(i);
   }
  }
 }
 inline void pmm_init() {
  init_bitmap();
  uint64_t kstart = (uint64_t)(kernel_start - KBASE);
  uint64_t kend   = (uint64_t)(kernel_end - KBASE);
  reserve_region(0, PAGE_SIZE);
  reserve_region(kstart, kend - kstart);
 }
}
#endif
