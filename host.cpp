#include <unistd.h>
#include <csignal>

#include "debug.hpp"
#include "dev.hpp"
#include "kvm.hpp"
#include "misc.hpp"
#include "setup.hpp"
#include "monitor.hpp"
#include "irqchip.hpp"
std::atomic<bool> stop(false);
static int runUntilHalt(KvmVcpu& vcpu, IoBus& bus, MmioBus& mmioBus) {
 for (;;) {
  switch (vcpu.run()) {
   case KVM_EXIT_IO:
    bus.handle(*vcpu.runStruct());
    if(stop) {
     // std::cout << "Hit\n";
     return 0;
    }
    break;
   case KVM_EXIT_HLT:
    std::cout << "halt\n";
    return 0;
   case KVM_EXIT_DEBUG:
    // dumpGpr(vcpu);
    // dumpSegs(vcpu);
    dumpRip(vcpu);
    break;
   case KVM_EXIT_MMIO:
    mmioBus.handle(*vcpu.runStruct());
    break;
   case KVM_EXIT_SHUTDOWN: {
    std::cout << "triple fault\n";
    auto* r = vcpu.runStruct();
    std::cout << "suberror=" << r->internal.suberror << " ndata=" << r->internal.ndata << "\n";
    dumpRip(vcpu);
    return 0;
   }
   case KVM_EXIT_INTR: {
   	break;
   }
   case KVM_EXIT_INTERNAL_ERROR: {
    std::cout << "kvm internal error\n";
    auto* r = vcpu.runStruct();
    std::cout << "suberror=" << r->internal.suberror << " ndata=" << r->internal.ndata << "\n";
    return 0;
   }
   default:
    std::cerr << "unhandled exit: " << vcpu.runStruct()->exit_reason
              << "\n";
    return EXIT_FAILURE;
   }
  }
}
int dowork(int argc, char* argv[]) {
 if (argc != 2) {
  std::cerr << "invalid args\n";
  return 1;
 }
 struct sigaction sa = {};
 sa.sa_handler = [](int){};
 sigaction(SIGUSR1, &sa, nullptr);
 std::string binary(argv[1]);
 constexpr uint64_t MEM_SIZE = 32ULL * 1024 * 1024;
 constexpr uint64_t memStart = 1ULL * 1024 * 1024;
 try {
  IoBus bus;
  Serial com1(0x3f8);
  bus.registerDevice(0x3f8, 8, &com1);
  MmioBus mmioBus;
  MmioConsole con;
  mmioBus.registerDevice(0x20000, 0x1000, &con);
  KvmHandle kvm;
  KvmVm vm(kvm);
  ExitDevice exitDev(stop);
  bus.registerDevice(0x501, 1, &exitDev);
  Irq::setupIrqChip(vm.getFd());
  uint8_t* mem = vm.mapMemory(memStart, MEM_SIZE);
  vm.loadBinary(binary.c_str(), mem, 0);
  auto vcpu = vm.createVcpu();
  auto cpuid = kvm.supportedCpuid();
  vcpu.setCpuid(cpuid);
  setupProtectedMode(vcpu, memStart);
  vcpu.singleStep(false);
  int rc = runUntilHalt(vcpu, bus, mmioBus);
  // printf("H");
  Monitor mon(vcpu, mem, memStart, MEM_SIZE);
  // dumpGpr(vcpu);
  // dumpSegs(vcpu);
  // dumpGdt(vcpu);
  // mon.exec("info registers");
  mon.exec("info mem");
  // mon.exec("x/4g 0x500000");
  // std::cout << (mon.atHlt() == true? "HLT=1": "HLT=0");
  return rc;
 } catch (const std::exception& e) {
   std::cerr << e.what() << "\n";
   return EXIT_FAILURE;
  }
}
int main(int argc, char* argv[]) {
 return dowork(argc, argv);
}
