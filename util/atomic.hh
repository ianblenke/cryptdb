#pragma once

#include <stdint.h>

class atomic_u64 {
public:
  atomic_u64() : _ctr(0) {}
  uint64_t get_and_incr() {
    return __sync_fetch_and_add(&_ctr, 1);
  }
private:
  volatile uint64_t _ctr;
};
