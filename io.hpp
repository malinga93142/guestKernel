#pragma once
#include "kvm.hpp"
struct IoDevice {
 public:
  virtual ~IoDevice() = default;
  virtual void out(uint16_t port, uint8_t size, uint32_t val) = 0;
  virtual uint32_t in(uint16_t port, uint8_t size) = 0;
  virtual void outb(uint16_t port, uint8_t val) = 0;
  virtual void outw(uint16_t port, uint16_t val) = 0;
  virtual void outl(uint16_t port, uint32_t val) = 0;

  virtual uint8_t inb(uint16_t port) = 0;
  virtual uint16_t inw(uint16_t port) = 0;
  virtual uint32_t inl(uint16_t port) = 0;
};

class IoBus {
 public:
  IoBus() : ports_(kNumPorts, nullptr) {}
  void registerDevice(uint16_t base, uint32_t count, IoDevice* dev) {
    if (uint32_t(base) + count > kNumPorts)
      throw std::runtime_error("io range out of bounds");
    for (uint32_t i = 0; i < count; ++i)
      if (ports_[base + i]) throw std::runtime_error("io port already taken");
    for (uint32_t i = 0; i < count; ++i) ports_[base + i] = dev;
  }
  void handle(kvm_run& run) {
    auto& io = run.io;
    uint8_t* data = reinterpret_cast<uint8_t*>(&run) + io.data_offset;
    for (uint32_t i = 0; i < io.count; ++i) {
      uint8_t* p = data + size_t(i) * io.size;
      IoDevice* dev = ports_[io.port];
      if (io.direction == KVM_EXIT_IO_OUT) {
        // uint32_t v = 0;
        // std::memcpy(&v, p, io.size);
        // if (dev) dev->out(io.port, io.size, v);
        if (dev) {
          if (io.size == 1)
            dev->outb(io.port, *p);
          else if (io.size == 2) {
            uint16_t val = 0;
            val |= (p[1] << 0x8) | p[0];
            dev->outw(io.port, val);
          } else if (io.size == 4) {
            uint32_t val = 0;
            val |= (p[3] << 0x18) | (p[2] << 0x10) | (p[1] << 0x8) | p[0];
            dev->outl(io.port, val);
          }
        }
      } else {
        uint32_t v = dev ? dev->in(io.port, io.size) : allOnes(io.size);
        std::memcpy(p, &v, io.size);
      }
    }
  }

 private:
  static constexpr uint32_t kNumPorts = 65536;
  static uint32_t allOnes(uint8_t size) {
    return size >= 4 ? 0xFFFFFFFFU : (1u << (8 * size)) - 1;
  }
  std::vector<IoDevice*> ports_;
};

struct MmioDevice {
 public:
  virtual ~MmioDevice() = default;
  virtual uint64_t read(uint64_t offset, uint8_t size) = 0;
  virtual void write(uint64_t offset, uint64_t value, uint8_t size) = 0;
};

class MmioBus {
 public:
  void registerDevice(uint64_t base, uint64_t size, MmioDevice* dev) {
    for (auto& r : regions_) {
      if (base < r.base + r.size && r.base < base + size) {
        throw std::runtime_error("mmio range overlap");
      }
    }
    regions_.push_back({base, size, dev});
  }
  void handle(kvm_run& run) {
    auto& m = run.mmio;
    const Region* r = find(m.phys_addr);
    if (m.is_write) {
      uint64_t v = 0;
      std::memcpy(&v, m.data, m.len);
      if (r) {
        r->dev->write(m.phys_addr - r->base, v, m.len);
      }
    } else {
      uint64_t v = r ? r->dev->read(m.phys_addr - r->base, m.len)
                     : ~0ULL >> (64 - 8 * m.len);
      std::memcpy(m.data, &v, m.len);
    }
  }

 private:
  struct Region {
    uint64_t base;
    uint64_t size;
    MmioDevice* dev;
  };
  const Region* find(uint64_t addr) const {
    for (auto& r : regions_) {
      if (addr >= r.base && addr - r.base < r.size) return &r;
    }
    return nullptr;
  }
  std::vector<Region> regions_;
};
