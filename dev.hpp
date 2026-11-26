#pragma once
#include <iostream>

#include "io.hpp"
#include <atomic>
class Serial : public IoDevice {
 public:
  explicit Serial(uint16_t base) : base_(base) {}
  void out(uint16_t port, uint8_t, uint32_t value) override {
    if (port == base_) std::cout.put(char(value)).flush();
  }
  uint32_t in(uint16_t port, uint8_t) override {
    if (port == base_ + 0x5) return 0x20;
    return 0;
  }
  void outb(uint16_t port, uint8_t val) override {
    if (port == base_) {
      std::cout.put(val).flush();
      // printf("%c", val);
    }
  }
  void outw(uint16_t port, uint16_t val) override {
    if (port == base_) {
      std::cout.put(char((val >> 8) & 0xFF)).flush();
      std::cout.put(char(val & 0xFF)).flush();
    }
  }
  void outl(uint16_t port, uint32_t val) override {
    if (port == base_) {
      std::cout.put(char(val >> 24) & 0xFF).flush();
      std::cout.put(char(val >> 16) & 0xFF).flush();
      std::cout.put(char(val >> 8) & 0xFF).flush();
      std::cout.put(char(val) & 0xFF).flush();
    }
  }
  uint8_t inb(uint16_t port) override {
    (void)port;
    return 0;
  }
  uint16_t inw(uint16_t port) override {
    (void)port;
    return 0;
  }
  uint32_t inl(uint16_t port) override {
    (void)port;
    return 0;
  }

 private:
  uint16_t base_;
};
class MmioConsole : public MmioDevice {
 public:
  void write(uint64_t offset, uint64_t v, uint8_t) override {
    if (offset == 0) std::cout.put(char(v)).flush();
  }
  uint64_t read(uint64_t, uint8_t) override { return 0; }
};

class ExitDevice : public IoDevice {
 public:
  explicit ExitDevice(std::atomic<bool>& stop) : stop_(stop) {}
  void out(uint16_t, uint8_t, uint32_t v) override { code = v; stop_ = true; }
  uint32_t in(uint16_t, uint8_t) override { return 0; }
  void outb(uint16_t port, uint8_t val)  override { code = val; stop_ = true; }
  void outw(uint16_t port, uint16_t val) override {(void)port; (void)val;}
  void outl(uint16_t port, uint32_t val) override {(void)port; (void)val;}

  uint8_t inb(uint16_t port)  override{ return 0; }
  uint16_t inw(uint16_t port) override{ return 0; }
  uint32_t inl(uint16_t port) override {return 0; }

  uint32_t code = 0;
 private:
  std::atomic<bool>& stop_;
};
