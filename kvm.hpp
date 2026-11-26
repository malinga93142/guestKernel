#pragma once
#include <asm/kvm.h>
#include <fcntl.h>
#include <linux/kvm.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <cerrno>
#include <cstdint>
#include <vector>

#include "fd.hpp"

class KvmHandle {
 public:
  KvmHandle() {
    int fd = open("/dev/kvm", O_RDWR);
    check(fd >= 0, "open /dev/kvm");
    fd_ = Fd(fd);
    int api = ioctl(fd_.get(), KVM_GET_API_VERSION, 0);
    check(api >= 0, "KVM_GET_API_VERSION");
    if (api != KVM_API_VERSION)
      throw std::runtime_error(
          "KVM API version mismatch: kernel=" + std::to_string(api) +
          " header=" + std::to_string(KVM_API_VERSION));
  }
  int createVm() {
    int vmfd = ioctl(fd_.get(), KVM_CREATE_VM, 0);
    check(vmfd >= 0, "KVM_CREATE_VM");
    return vmfd;
  }
  std::vector<uint8_t> supportedCpuid() {
   constexpr int N = 256;
   std::vector<uint8_t> buf(sizeof(kvm_cpuid2) + N * sizeof(kvm_cpuid_entry2));
   auto *c = reinterpret_cast<kvm_cpuid2 *>(buf.data());
   c->nent = N;
   check(ioctl(fd_.get(), KVM_GET_SUPPORTED_CPUID, c) == 0, "KVM_GET_SUPPORTED_CPUID");
   return buf;
  }
  int vcpuMmapSize() {
    int size = ioctl(fd_.get(), KVM_GET_VCPU_MMAP_SIZE, 0);
    check(size > 0, "KVM_GET_VCPU_MMAP_SIZE");
    return size;
  }

 private:
  Fd fd_;
};

class KvmVcpu {
 public:
  KvmVcpu(int vcpufd, int mmapSize) : fd_(vcpufd), mmapSize_(mmapSize) {
    void* p = mmap(nullptr, mmapSize_, PROT_READ | PROT_WRITE, MAP_SHARED,
                   fd_.get(), 0);
    check(p != MAP_FAILED, "mmap kvm_run");
    run_ = static_cast<kvm_run*>(p);
  }
  KvmVcpu(const KvmVcpu&) = delete;
  KvmVcpu& operator=(const KvmVcpu&) = delete;
  KvmVcpu(KvmVcpu&&) = delete;
  ~KvmVcpu() {
    if (run_) munmap(run_, mmapSize_);
  }

  void getRegs(kvm_regs& r) {
    check(ioctl(fd_.get(), KVM_GET_REGS, &r) == 0, "KVM_GET_REGS");
  }
  void setRegs(const kvm_regs& r) {
    check(ioctl(fd_.get(), KVM_SET_REGS, &r) == 0, "KVM_SET_REGS");
  }
  void getSregs(kvm_sregs& s) {
    check(ioctl(fd_.get(), KVM_GET_SREGS, &s) == 0, "KVM_GET_SREGS");
  }
  void setSregs(const kvm_sregs& s) {
    check(ioctl(fd_.get(), KVM_SET_SREGS, &s) == 0, "KVM_SET_SREGS");
  }

  void singleStep(bool on) {
    kvm_guest_debug dbg = {};
    dbg.control = KVM_GUESTDBG_ENABLE | (on ? KVM_GUESTDBG_SINGLESTEP : 0);
    check(ioctl(fd_.get(), KVM_SET_GUEST_DEBUG, &dbg) == 0,
          "KVM_SET_GUEST_DEBUG");
  }
  void setCpuid(std::vector<uint8_t>& buf) {
   check(ioctl(fd_.get(), KVM_SET_CPUID2, buf.data()) == 0, "KVM_SET_CPUID2");
  }
  int run() {
   int r = ioctl(fd_.get(), KVM_RUN, 0);
   if (r < 0 && errno == EINTR) return KVM_EXIT_INTR;
   check(r == 0, "KVM_RUN");
   return run_->exit_reason;
  }
  void requestExit() { __atomic_store_n(&run_->immediate_exit, 1, __ATOMIC_RELAXED); }
  void clearExit() { __atomic_store_n(&run_->immediate_exit, 0, __ATOMIC_RELAXED); }
  kvm_run* runStruct() { return run_; }

 private:
  Fd fd_;
  int mmapSize_;
  kvm_run* run_ = nullptr;
};

class KvmVm {
 public:
  explicit KvmVm(KvmHandle& kvm)
      : fd_(kvm.createVm()), vcpuMmapSize_(kvm.vcpuMmapSize()) {}
  KvmVm(const KvmVm&) = delete;
  KvmVm& operator=(const KvmVm&) = delete;
  ~KvmVm() {
   for (auto& s : slots_) munmap(s.mem, s.size);
  }

  uint8_t* mapSlot(uint32_t slot, uint64_t guest_phys, uint64_t size) {
   void* mem = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
   check(mem != MAP_FAILED, "mmap guest memory");
   kvm_userspace_memory_region region = {
     .slot = slot,
     .guest_phys_addr = guest_phys,
     .memory_size = size,
     .userspace_addr = (uint64_t)mem,
   };
   check(ioctl(fd_.get(), KVM_SET_USER_MEMORY_REGION, &region) == 0, "KVM_SET_USER_MEMORY_REGION");
   slots_.push_back({(uint8_t*)mem, size});
   return (uint8_t*)mem;
  }
  uint8_t* mapMemory(uint64_t __start, uint64_t size) { return mapSlot(0, __start, size); }

  void loadBinary(const char* path, uint8_t* mem, uint64_t offset) {
   Fd img(open(path, O_RDONLY));
   check(img.valid(), "open guest image");
   struct stat st;
   check(fstat(img.get(), &st) == 0, "fstat guest image");
   ssize_t n = read(img.get(), mem + offset, st.st_size);
   check(n == st.st_size, "read guest image");
  }

  KvmVcpu createVcpu(uint32_t id = 0) {
   int vcpufd = ioctl(fd_.get(), KVM_CREATE_VCPU, id);
   check(vcpufd >= 0, "KVM_CREATE_VCPU");
   return KvmVcpu(vcpufd, vcpuMmapSize_);
  }
  int getFd() {
   return fd_.get();
  }
 private:
  struct Slot {
   uint8_t* mem;
   uint64_t size;
  };
  Fd fd_;
  int vcpuMmapSize_;
  std::vector<Slot> slots_;
};
