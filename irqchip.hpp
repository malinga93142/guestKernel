#include <linux/kvm.h>
#include <sys/ioctl.h>
#include <stdexcept>
#include <cstring>
#include <cerrno>
#include <string>
#include "kvm.hpp"
namespace Irq {
 inline void check(int rc, const char *what) {
  if(rc < 0) std::runtime_error(std::string(what) + ": " + strerror(errno));
 }
 inline void setupIrqChip(int vmFd, bool withPit = true) {
  check(ioctl(vmFd, KVM_CREATE_IRQCHIP, 0), "KVM_CREATE_IRQCHIP");
  if (withPit) {
   kvm_pit_config pit{};
   pit.flags = KVM_PIT_SPEAKER_DUMMY;
   Irq::check(ioctl(vmFd, KVM_CREATE_PIT2, &pit), "KVM_CREATE_PIT2");
  }
 }
 inline void setIrqLine(int vmFd, uint32_t gsi, uint32_t level) {
  kvm_irq_level l{};
  l.irq = gsi;
  l.level = level;
  Irq::check(ioctl(vmFd, KVM_IRQ_LINE, &l), "KVM_IRQ_LINE");
 }
 inline bool isHalted(int vcpuFd) {
  kvm_mp_state mp{};
  if (ioctl(vcpuFd, KVM_GET_MP_STATE, &mp) < 0) return false;
  return mp.mp_state == KVM_MP_STATE_HALTED;
 }
}
