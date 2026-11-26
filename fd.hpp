#pragma once
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>

inline void check(bool ok, const char* what) {
  if (!ok) throw std::runtime_error(std::string(what) + ": " + strerror(errno));
}

class Fd {
 public:
  Fd() = default;
  explicit Fd(int fd) : fd_(fd) {}
  Fd(const Fd&) = delete;
  Fd& operator=(const Fd&) = delete;
  Fd(Fd&& o) noexcept : fd_(o.fd_) { o.fd_ = -1; }
  Fd& operator=(Fd&& o) noexcept {
    if (this != &o) {
      closeIfValid();
      fd_ = o.fd_;
      o.fd_ = -1;
    }
    return *this;
  }
  ~Fd() { closeIfValid(); }
  int get() const { return fd_; }
  bool valid() const { return fd_ >= 0; }

 private:
  void closeIfValid() {
    if (fd_ >= 0) ::close(fd_);
  }
  int fd_ = -1;
};