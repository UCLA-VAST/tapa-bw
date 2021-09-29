#include <ap_int.h>
#include <tapa.h>

#ifndef PORT_WIDTH
#error PORT_WIDTH must be defined
#endif

#if PORT_WIDTH != 32 && PORT_WIDTH != 64 && PORT_WIDTH != 128 && \
    PORT_WIDTH != 256 && PORT_WIDTH != 512
#error invalid PORT_WIDTH
#endif

#ifndef CONCURRENCY
#error CONCURRENCY must be defined
#endif

using Elem = ap_uint<PORT_WIDTH>;

// Generate random addresses with (burst_m1 + 1) burst length.
class RandAddr {
 public:
  static constexpr int kWidth = 21;
  static constexpr unsigned kDefaultInitLfsr = 0xdecafu;
  using Addr = ap_uint<kWidth>;
  RandAddr(Addr burst_m1, Addr init_lfsr = kDefaultInitLfsr, Addr init_addr = 0)
      : burst_(burst_m1),
        lfsr_(init_lfsr),
        addr_(init_addr),
        addr_end_(init_addr + burst_m1) {
    CHECK_NE(lfsr_, 0);
  }
  operator Addr() const { return addr_; }
  Addr operator++(int) {
    const auto old_addr = addr_;
    if (addr_ == addr_end_) {
      addr_ = lfsr_;
      addr_end_ = lfsr_ + burst_;
    } else {
      ++addr_;
    }

    lfsr_.set_bit(0, (lfsr_ & 0x5u).xor_reduce());
    lfsr_.rrotate(1);
    return old_addr;
  }

 private:
  const Addr burst_;
  Addr lfsr_;
  Addr addr_;
  Addr addr_end_;
};
