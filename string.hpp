#ifndef STRING_HPP
#define STRING_HPP
#include <cstdint>
namespace String {
 inline void *memCpy(void *dest, const void *src, unsigned long len) {
  unsigned char *__d = (unsigned char*)dest;
  unsigned char *__s = (unsigned char*)src;
  while (len-- > 0) {
   *__d++ = *__s++;
  }
  return dest;
 }
 inline void *memSet(void *dest, uint8_t byte, unsigned long len) {
  unsigned char *__d = (unsigned char *)dest;
  while (len-- > 0)
   *__d++ = byte;
  return dest;
 }
}
#endif