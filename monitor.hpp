// monitor.hpp
#pragma once
#include <cctype>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>
#include "debug.hpp"
class Monitor {
 public:
  Monitor(KvmVcpu& v, uint8_t* host, uint64_t gpaBase, uint64_t size)
      : v_(v), host_(host), base_(gpaBase), size_(size) {}

  void exec(const std::string& c) {
    if (c == "info registers") { dumpGpr(v_); dumpSegs(v_); }
    else if (c == "info mem")  infoMem();
    else if (c.rfind("x/", 0) == 0) examine(c);
    else printf("unknown command: %s\n", c.c_str());
  }
  bool atHlt() {
   return atHalt();
  }
 private:
  static constexpr uint64_t ADDR = 0x000ffffffffff000ULL, NX = 1ULL << 63;
  KvmVcpu& v_; uint8_t* host_; uint64_t base_, size_;
  bool rd(uint64_t gpa, void* out, size_t n) {  // guest-physical read
    if (gpa < base_ || gpa + n > base_ + size_) return false;
    memcpy(out, host_ + (gpa - base_), n);
    return true;
  }

  // Walk 4-level tables. fn(va, pa, size, flags) is called per leaf mapping.
  // flags: bit1=rw, bit2=user (ANDed down the walk), bit63=NX (ORed).
  template <class F>
  void walk(uint64_t table, int lvl, uint64_t va, uint64_t acc, F& fn) {
    for (uint64_t i = 0; i < 512; i++) {
      uint64_t e;
      if (!rd(table + i * 8, &e, 8) || !(e & 1)) continue;
      uint64_t sz = 1ULL << (12 + 9 * (lvl - 1)), a = va | (i * sz);
      uint64_t f = ((acc & e) & 6) | ((acc | e) & NX);
      if (lvl == 1 || (lvl <= 3 && (e & 0x80)))
        fn(a, e & ADDR & ~(sz - 1), sz, f);
      else
        walk(e & ADDR, lvl - 1, a, f, fn);
    }
  }
  bool translate(uint64_t va, uint64_t& pa) {   // guest virtual -> guest physical
    kvm_sregs s; v_.getSregs(s);
    if (!(s.cr0 >> 31 & 1)) { pa = va; return true; }
    uint64_t t = s.cr3 & ADDR;
    for (int l = 4; l >= 1; l--) {
      uint64_t e, sz = 1ULL << (12 + 9 * (l - 1));
      if (!rd(t + ((va / sz) & 511) * 8, &e, 8) || !(e & 1)) return false;
      if (l == 1 || (l <= 3 && (e & 0x80))) { pa = (e & ADDR & ~(sz - 1)) | (va & (sz - 1)); return true; }
      t = e & ADDR;
    }
    return false;
  }

  void infoMem() {
    kvm_sregs s; v_.getSregs(s);
    if (!(s.cr0 >> 31 & 1)) { puts("paging off: virtual == physical"); return; }
    if (!(s.efer >> 10 & 1)) { puts("only long-mode paging supported"); return; }
    uint64_t va = 0, pa = 0, len = 0, fl = 0;     // pending merged range
    auto flush = [&] {
      if (len) printf("%016llx-%016llx -> pa %016llx %c%c%c\n",
                      (unsigned long long)va, (unsigned long long)(va + len),
                      (unsigned long long)pa, fl & 4 ? 'u' : '-',
                      fl & 2 ? 'w' : 'r', fl & NX ? '-' : 'x');
    };
    auto fn = [&](uint64_t a, uint64_t p, uint64_t sz, uint64_t f) {
      if (len && a == va + len && p == pa + len && f == fl) { len += sz; return; }
      flush(); va = a; pa = p; len = sz; fl = f;
    };
    walk(s.cr3 & ADDR, 4, 0, 6, fn);
    flush();
  }

  // x/[count][x|d|c][b|h|w|g] addr   (addr is guest virtual)
  void examine(const std::string& c) {
    std::istringstream in(c.substr(2));
    size_t count = 1; char fmt = 'x'; int unit = 4;
    if (isdigit(in.peek())) in >> count;
    for (char ch; isalpha(in.peek()) && in.get(ch);) {
      if (ch == 'x' || ch == 'd' || ch == 'c') fmt = ch;
      else if (ch == 'b') unit = 1; else if (ch == 'h') unit = 2;
      else if (ch == 'w') unit = 4; else if (ch == 'g') unit = 8;
      else { printf("bad format '%c'\n", ch); return; }
    }
    if (fmt == 'c') unit = 1;
    uint64_t addr; in >> std::ws >> std::hex;     // accepts "0x..." too
    std::string a; in >> a;
    addr = strtoull(a.c_str(), nullptr, 0);
    size_t perLine = fmt == 'c' ? 8 : 16 / unit;
    for (size_t n = 0; n < count; n++) {
      uint64_t va = addr + n * unit, pa, val = 0;
      if (n % perLine == 0) printf("%s%016llx:", n ? "\n" : "", (unsigned long long)va);
      if (!translate(va, pa) || !rd(pa, &val, unit)) {
        printf("\nCannot access memory at %#llx\n", (unsigned long long)va);
        return;
      }
      int64_t sv = (int64_t)(val << (64 - unit * 8)) >> (64 - unit * 8);
      if (fmt == 'x')      printf(" 0x%0*llx", unit * 2, (unsigned long long)val);
      else if (fmt == 'd') printf(" %lld", (long long)sv);
      else                 printf(" %d '%c'", (int)sv, isprint((int)val) ? (int)val : '.');
    }
    puts("");
  }
  bool atHalt() {
   kvm_regs r; v_.getRegs(r);
   uint64_t pa; uint8_t op = 0;
   return translate(r.rip, pa) && rd(pa, &op, 1) && op == 0xf4;
  }
};
