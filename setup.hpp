#pragma once
#include <asm/kvm.h>
#include "kvm.hpp"

inline void setupRealMode(KvmVcpu& vcpu, uint64_t rip, uint64_t rsp) {
  kvm_sregs sregs;
  vcpu.getSregs(sregs);
  sregs.cs.base = 0;
  sregs.cs.selector = 0;
  vcpu.setSregs(sregs);

  kvm_regs regs;
  vcpu.getRegs(regs);
  regs.rip = rip;
  regs.rsp = rsp;
  vcpu.setRegs(regs);
}
namespace detail {
  inline kvm_segment flatSeg(uint16_t selector, bool code) {
    kvm_segment s = {};
    s.base = 0;
    s.limit = 0xFFFFFFFF;
    s.selector = selector;
    s.type = code ? 0xb: 0x3;
    s.present = 1; s.dpl = 0; s.db = 1; s.s = 1; s.g = 1;
    return s;
  }
}

inline void setupProtectedMode(KvmVcpu& vcpu, uint32_t entry) {
 kvm_sregs sregs;
 vcpu.getSregs(sregs);

 sregs.cr0 = (sregs.cr0 | 1u) & ~(1u << 31);
 sregs.cr3 = 0;
 sregs.cr4 = 0;
 sregs.efer= 0;

 sregs.cs = detail::flatSeg(0x08, true);
 sregs.ds = sregs.es = sregs.fs = sregs.gs = sregs.ss = detail::flatSeg(0x10, false);

 sregs.gdt.base = 0;
 sregs.gdt.limit= 0;
 sregs.idt.base = 0;
 sregs.idt.limit= 0;
 vcpu.setSregs(sregs);

 kvm_regs regs = {};
 regs.rip = entry;
 regs.rflags = 0x2; 
 vcpu.setRegs(regs);
}