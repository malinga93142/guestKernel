#include "exceptions.hpp"
extern "C" void handler(struct Exceptions::InterruptFrame *frame) {
  handle_interrupt(frame);
}
