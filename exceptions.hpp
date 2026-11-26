#ifndef EXCEPTIONS_HPP
#define EXCEPTIONS_HPP
#ifdef __cplusplus
namespace Exceptions {
 typedef unsigned long uint64_t;
 struct InterruptFrame {
  uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
  uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
  uint64_t rip, cs, rflag, rsp, ss;
 } __attribute__((packed));

 inline void handle_interrupt(InterruptFrame *frame) {
  (void)frame;
 }
 // extern "C" {
 //  void isr0();
 //  void isr1();
 //  void isr2();
 //  void isr3();
 //  void isr4();
 //  void isr5();
 //  void isr6();
 //  void isr7();
 //  void isr8();
 //  void isr9();
 //  void isr10();
 //  void isr11();
 //  void isr12();
 //  void isr13();
 //  void isr14();
 //  void isr15();
 //  void isr16();
 //  void isr17();
 //  void isr18();
 //  void isr19();
 //  void isr20();
 //  void isr21();
 //  void isr22();
 //  void isr23();
 //  void isr24();
 //  void isr25();
 //  void isr26();
 //  void isr27();
 //  void isr28();
 //  void isr29();
 //  void isr30();
 //  void isr31();
 // }
}
#endif
#endif