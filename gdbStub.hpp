#pragma once
#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/kvm.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <functional>
#include <string>
#include <vector>
#include "monitor.hpp"
enum class Stop { Debug, Halt, Shutdown, Intr, Other };

// run_ : performs ONE KVM_RUN, services IO/MMIO exits itself, and returns
//        why it stopped (Intr when KVM_RUN failed with EINTR).
class GdbStub {
 public:
  GdbStub(KvmVcpu& v, Monitor& m, std::function<Stop()> run)
   : v_(v), mon_(m), run_(std::move(run)) {}
  void serve(int port = 1234) {
   int ls = socket(AF_INET, SOCK_STREAM, 0), one = 1;
   setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
   sockaddr_in a{}; a.sin_family = AF_INET; a.sin_port = htons(port);
   a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
   if (bind(ls, (sockaddr*)&a, sizeof a) < 0 || listen(ls, 1) < 0) { perror("gdbstub"); return; }
   fprintf(stderr, "waiting for gdb on :%d\n", port);
   fd_ = accept(ls, nullptr, nullptr);
   close(ls);
   fcntl(fd_, F_SETOWN, getpid());             // SIGIO on incoming data (Ctrl-C)
   std::string p;
   while (recvPkt(p) && handle(p)) {}
   close(fd_);
  }
 private:
  KvmVcpu& v_; Monitor& mon_; std::function<Stop()> run_;
  int fd_ = -1; std::vector<uint64_t> bps_;
  // wire
  bool recvPkt(std::string& out) {
   char c;
   do { if (read(fd_, &c, 1) <= 0) return false; } while (c != '$');
   out.clear();
   while (read(fd_, &c, 1) == 1 && c != '#') out += c;
   char ck[2]; recv(fd_, ck, 2, MSG_WAITALL);  // checksum not verified
   write(fd_, "+", 1);
   return true;
  }
  void send(const std::string& s) {
   unsigned cs = 0; for (unsigned char c : s) cs += c;
   char t[4]; snprintf(t, sizeof t, "#%02x", cs & 0xff);
   std::string p = "$" + s + t;
   ::send(fd_, p.data(), p.size(), 0);
  }
  static std::string hexle(uint64_t v, int n) {
   std::string s; char b[3];
   for (int i = 0; i < n; i++) { snprintf(b, 3, "%02x", (unsigned)(v >> (8 * i)) & 0xff); s += b; }
   return s;
  }
  static uint64_t unhexle(const char* p, int n) {
   uint64_t v = 0; unsigned b;
   for (int i = 0; i < n; i++) { sscanf(p + 2 * i, "%2x", &b); v |= (uint64_t)b << (8 * i); }
   return v;
  }
  //registers: gdb's default amd64 order
  static std::vector<uint64_t*> gp(kvm_regs& r) {
   return {&r.rax, &r.rbx, &r.rcx, &r.rdx, &r.rsi, &r.rdi, &r.rbp, &r.rsp,
           &r.r8, &r.r9, &r.r10, &r.r11, &r.r12, &r.r13, &r.r14, &r.r15, &r.rip};
  }
  std::string readRegs() {
   kvm_regs r; kvm_sregs s;
   ioctl(v_.fd(), KVM_GET_REGS, &r); ioctl(v_.fd(), KVM_GET_SREGS, &s);
   std::string o;
   for (auto* p : gp(r)) o += hexle(*p, 8);
   o += hexle(r.rflags, 4);
   for (auto sel : {s.cs.selector, s.ss.selector, s.ds.selector,
                    s.es.selector, s.fs.selector, s.gs.selector})
    o += hexle(sel, 4);
   return o;
  }
  void writeRegs(const char* h) {
   kvm_regs r; ioctl(v_.fd(), KVM_GET_REGS, &r);
   for (auto* p : gp(r)) { *p = unhexle(h, 8); h += 16; }
   r.rflags = unhexle(h, 4);                  // segment selectors ignored
   ioctl(v_.fd(), KVM_SET_REGS, &r);
  }

  // memory (gdb addresses are guest virtual)
  std::string readMem(uint64_t a, size_t n) {
   std::string o;
   for (size_t i = 0; i < n; i++) {
    uint64_t pa; uint8_t b;
    if (!mon_.translate(a + i, pa) || !mon_.rd(pa, &b, 1)) break;
    o += hexle(b, 1);
   }
   return o.empty() && n ? "E01" : o;
  }
  bool writeMem(uint64_t a, size_t n, const char* h) {
   for (size_t i = 0; i < n; i++) {
    uint64_t pa; uint8_t b = unhexle(h + 2 * i, 1);
    if (!mon_.translate(a + i, pa) || !mon_.wr(pa, &b, 1)) return false;
   }
   return true;
  }

  // execution control
  void applyDebug(bool step) {
   kvm_guest_debug d{};
   d.control = KVM_GUESTDBG_ENABLE | (step ? KVM_GUESTDBG_SINGLESTEP : 0);
   if (!bps_.empty()) {
    d.control |= KVM_GUESTDBG_USE_HW_BP;
    for (size_t i = 0; i < bps_.size(); i++) {
     d.arch.debugreg[i] = bps_[i];
     d.arch.debugreg[7] |= 1ULL << (2 * i);   // local enable, exec, len 1
    }
   }
   ioctl(v_.fd(), KVM_SET_GUEST_DEBUG, &d);
  }
  bool ctrlC() {
   char c;
   if (recv(fd_, &c, 1, MSG_DONTWAIT) == 1 && c == 0x03) return true;
   return false;
  }
  std::string resume(bool step) {
   applyDebug(step);
   if (mon_.atHlt()) return "S05";             // would block forever in-kernel
   for (;;) {
    switch (run_()) {
     case Stop::Intr:     if (ctrlC()) return "S02"; break;
     case Stop::Debug:
     case Stop::Halt:     return "S05";
     default:             return "S0b";
    }
   }
  }

  bool handle(const std::string& p) {
   const char* a = p.c_str() + 1;
   switch (p[0]) {
    case '?': send("S05"); break;
    case 'g': send(readRegs()); break;
    case 'G': writeRegs(a); send("OK"); break;
    case 'm': { uint64_t ad; size_t n; sscanf(a, "%lx,%zx", &ad, &n); send(readMem(ad, n)); break; }
    case 'M': { uint64_t ad; size_t n; int off;
                sscanf(a, "%lx,%zx:%n", &ad, &n, &off);
                send(writeMem(ad, n, a + off) ? "OK" : "E01"); break; }
    case 'c': send(resume(false)); break;
    case 's': send(resume(true)); break;
    case 'Z': case 'z': {
     unsigned t; uint64_t ad; sscanf(a, "%u,%lx", &t, &ad);
     if (t > 1) { send(""); break; }          // only exec breakpoints
     auto it = std::find(bps_.begin(), bps_.end(), ad);
     if (p[0] == 'Z') {
      if (it == bps_.end()) { if (bps_.size() >= 4) { send("E01"); break; } bps_.push_back(ad); }
     } else if (it != bps_.end()) bps_.erase(it);
     send("OK"); break;
    }
    case 'H': send("OK"); break;
    case 'D': send("OK"); return false;
    case 'k': return false;
    case 'q':
     if (p.rfind("qSupported", 0) == 0) send("PacketSize=4000");
     else if (p == "qAttached") send("1");
     else send("");
     break;
    default: send("");                            // unsupported -> empty reply
   }
   return true;
  }
};