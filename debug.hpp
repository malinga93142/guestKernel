#pragma once
#include <linux/kvm.h>
#include <iomanip>
#include <iostream>

#include "kvm.hpp"
#define CF_BIT 0
#define PF_BIT 2
#define AF_BIT 4
#define ZF_BIT 6
#define SF_BIT 7
#define TF_BIT 8
#define IF_BIT 9
#define DF_BIT 10
#define OF_BIT 11
#define IOPL_SHIFT 12
#define NT_BIT 14
#define RF_BIT 16
#define VM_BIT 17
#define AC_BIT 18
#define VIF_BIT 19
#define VIP_BIT 20
#define ID_BIT 21

#define CF (1UL << CF_BIT)
#define PF (1UL << PF_BIT)
#define AF (1UL << AF_BIT)
#define ZF (1UL << ZF_BIT)
#define SF (1UL << SF_BIT)
#define TF (1UL << TF_BIT)
#define IF (1UL << IF_BIT)
#define DF (1UL << DF_BIT)
#define OF (1UL << OF_BIT)
#define NT (1UL << NT_BIT)
#define RF (1UL << RF_BIT)
#define VM (1UL << VM_BIT)
#define AC (1UL << AC_BIT)
#define ID (1UL << ID_BIT)
#define VIF (1ULL << VIF_BIT)
#define VIP (1ULL << VIP_BIT)
#define IOPL(f) (((f) >> IOPL_SHIFT) & 3UL)

#define EFL_TEST(f, mask) (((f) & (mask)) != 0)

static inline void __dump_flags(uint64_t reg, uint64_t mask,
                                const char* __str) {
#define TEST_BIT(reg, mask) EFL_TEST(reg, mask)
  printf("%s", TEST_BIT(reg, mask) ? __str : "");
#undef TEST_BIT
}
static void dumpEflags(uint64_t flag_register) {
  printf("[ ");
  __dump_flags(flag_register, ID, "ID ");
  __dump_flags(flag_register, VIP, "VIP ");
  __dump_flags(flag_register, VIF, "VIF ");
  __dump_flags(flag_register, AC, "AC ");
  __dump_flags(flag_register, VM, "VM ");
  __dump_flags(flag_register, RF, "RF ");
  __dump_flags(flag_register, NT, "NT ");
  printf("%s", ((flag_register >> 12) & 3UL) != 0 ? "IOPL " : "");
  __dump_flags(flag_register, OF, "OF ");
  __dump_flags(flag_register, DF, "DF ");
  __dump_flags(flag_register, IF, "IF ");
  __dump_flags(flag_register, TF, "TF ");
  __dump_flags(flag_register, SF, "SF ");
  __dump_flags(flag_register, ZF, "ZF ");
  __dump_flags(flag_register, AF, "AF ");
  __dump_flags(flag_register, PF, "PF ");
  __dump_flags(flag_register, CF, "CF ");
  printf("]\n");
}

struct RegDumper {
#define NEWLINE "\n"
#define TWOSPACES "  "
  const kvm_regs& r;
  friend std::ostream& operator<<(std::ostream& os,
                                  const RegDumper& __regdump) {
    std::ios_base::fmtflags flags_backup(os.flags());
    char fill_backup = os.fill();
    os << std::hex << std::uppercase << std::setfill('0');
    os << "RAX= 0x" << std::setw(16) << __regdump.r.rax << TWOSPACES
       << "RBX= 0x" << std::setw(16) << __regdump.r.rbx << TWOSPACES
       << "RCX= 0x" << std::setw(16) << __regdump.r.rcx << TWOSPACES
       << "RDX= 0x" << std::setw(16) << __regdump.r.rdx << NEWLINE;
    os << "RSI= 0x" << std::setw(16) << __regdump.r.rsi << TWOSPACES
       << "RDI= 0x" << std::setw(16) << __regdump.r.rdi << TWOSPACES
       << "RBP= 0x" << std::setw(16) << __regdump.r.rbp << TWOSPACES
       << "RSP= 0x" << std::setw(16) << __regdump.r.rsp << NEWLINE;
    os << "R8 = 0x" << std::setw(16) << __regdump.r.r8 << TWOSPACES << "R9 = 0x"
       << std::setw(16) << __regdump.r.r9 << TWOSPACES << "R10= 0x"
       << std::setw(16) << __regdump.r.r10 << TWOSPACES << "R11= 0x"
       << std::setw(16) << __regdump.r.r11 << NEWLINE;
    os << "R12= 0x" << std::setw(16) << __regdump.r.r12 << TWOSPACES
       << "R13= 0x" << std::setw(16) << __regdump.r.r13 << TWOSPACES
       << "R14= 0x" << std::setw(16) << __regdump.r.r14 << TWOSPACES
       << "R15= 0x" << std::setw(16) << __regdump.r.r15 << NEWLINE;
    os << "RIP= 0x" << std::setw(16) << __regdump.r.rip << TWOSPACES
       << "RFL= 0x" << std::setw(16) << __regdump.r.rflags;

    os.flags(flags_backup);
    os.fill(fill_backup);
#undef NEWLINE
    return os;
  }
};
static inline void __dump_regs() {}
inline void dumpGpr(KvmVcpu& vcpu) {
  kvm_regs r;
  vcpu.getRegs(r);
  std::cout << RegDumper{r} << " ";
  dumpEflags(r.rflags);
}

struct SregDumper {
  const kvm_sregs& s;
  friend std::ostream& operator<<(std::ostream& os, const SregDumper& __sreg) {
    std::ios_base::fmtflags flags_backup(os.flags());
    char fill_backup = os.fill();
    os << std::hex << std::uppercase << std::setfill('0');
    auto print_seg = [&](const char* name, const kvm_segment& seg) {
      uint32_t raw_limit = seg.g ? (seg.limit >> 12) : (seg.limit & 0xFFFFF);
      os << std::setw(2) << name << "= " << std::setw(4) << seg.selector
         << " 0x" << std::setw(16) << seg.base << " " << std::setw(5)
         << raw_limit << " type=0x" << std::setw(2)
         << static_cast<int>(seg.type) << " dpl=" << static_cast<int>(seg.dpl)
         << " p=" << static_cast<int>(seg.present)
         << " l=" << static_cast<int>(seg.l)
         << " db=" << static_cast<int>(seg.db)
         << " g=" << static_cast<int>(seg.g) << "\n";
    };
    print_seg("CS", __sreg.s.cs);
    print_seg("DS", __sreg.s.ds);
    print_seg("ES", __sreg.s.es);
    print_seg("FS", __sreg.s.fs);
    print_seg("SS", __sreg.s.ss);
    print_seg("TR", __sreg.s.tr);
    print_seg("LDT", __sreg.s.ldt);
    auto print_dtable = [&](const char* name, const kvm_dtable& dt) {
      os << name << " 0x" << std::setw(16) << dt.base << " 0x"
         << std::setw(4) << static_cast<uint32_t>(dt.limit) << "\n";
    };

    print_dtable("GDT", __sreg.s.gdt);
    print_dtable("IDT", __sreg.s.idt);

    // 4. Control Registers & EFER
    os << "CR0 = 0x" << std::setw(16) << __sreg.s.cr0 << "  "
       << "CR2 = 0x" << std::setw(16) << __sreg.s.cr2 << "\n";
    os << "CR3 = 0x" << std::setw(16) << __sreg.s.cr3 << "  "
       << "CR4 = 0x" << std::setw(16) << __sreg.s.cr4 << "\n";
    os << "EFER= 0x" << std::setw(16) << __sreg.s.efer << "\n";
    os.flags(flags_backup);
    os.fill(fill_backup);
    return os;
  }
};
inline void dumpSegs(KvmVcpu& vcpu) {
  kvm_sregs s;
  vcpu.getSregs(s);
  std::cout << SregDumper{s};
}
inline void dumpRip(KvmVcpu &vcpu) {
	kvm_regs r;
	vcpu.getRegs(r);
	std::cout << std::hex << "RIP= 0x" << std::setfill('0') << std::setw(16) << r.rip << "\n";
}
inline void dumpGdt(KvmVcpu& vcpu) {
  kvm_sregs s;
  vcpu.getSregs(s);
  // std::cout << SregDumper{s};
}

// inline void memDebug(KvmVcpu& vcpu) {
//  std::string str;
//  std::cin.getline(str, );
// }
