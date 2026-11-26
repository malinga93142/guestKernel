#include <unistd.h>

#include <csignal>

#include "debug.hpp"
#include "dev.hpp"
#include "kvm.hpp"
#include "misc.hpp"
#include "setup.hpp"

static int runUntilHalt(KvmVcpu& vcpu, IoBus& bus, MmioBus& mmioBus) {
  for (;;) {
    switch (vcpu.run()) {
      case KVM_EXIT_IO:
        bus.handle(*vcpu.runStruct());
        break;
      case KVM_EXIT_HLT:
        std::cout << "halt\n";
        return 0;
      case KVM_EXIT_DEBUG:
        // dumpGpr(vcpu);
        // dumpSegs(vcpu);
        break;
      case KVM_EXIT_MMIO:
        mmioBus.handle(*vcpu.runStruct());
        break;
      case KVM_EXIT_SHUTDOWN:
        std::cout << "triple fault\n";
        return 0;
      case KVM_EXIT_INTERNAL_ERROR:
        std::cout << "kvm internal error\n";
        return 0;
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
  std::string binary(argv[1]);
  constexpr uint64_t MEM_SIZE = 1ULL * 1024 * 1024;
  try {
    IoBus bus;
    Serial com1(0x3f8);
    bus.registerDevice(0x3f8, 8, &com1);
    MmioBus mmioBus;
    MmioConsole con;
    mmioBus.registerDevice(0x20000, 0x1000, &con);
    KvmHandle kvm;
    KvmVm vm(kvm);
    uint8_t* mem = vm.mapMemory(MEM_SIZE);
    vm.loadBinary(binary.c_str(), mem, 0);
    auto vcpu = vm.createVcpu();
    setupRealMode(vcpu, 0x1000, MEM_SIZE);
    vcpu.singleStep(true);
    int rc = runUntilHalt(vcpu, bus, mmioBus);
    dumpGpr(vcpu);
    dumpSegs(vcpu);
    dumpGdt(vcpu);
    // sigqueue(getpid(), SIGINT, vcpu);
    return rc;
  } catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return EXIT_FAILURE;
  }
}
int main(int argc, char* argv[]) {
  return dowork(argc, argv);
}
