#include <ap_int.h>

#include <tapa.h>

#include "bw.h"

void ReadOnlyBenchmark(tapa::async_mmap<Elem> mem, int64_t n, int32_t burst,
                       RandAddr::Addr id) {
  RandAddr read_addr(burst, id, id);

spin:
  for (int64_t i_read_req = 0, i_read_resp = 0; i_read_resp < n;) {
#pragma HLS pipeline II = 1

    // send read request
    if (i_read_req < n && !mem.read_addr.full()) {
      mem.read_addr.try_write(read_addr++);
      ++i_read_req;
    }

    // collect read response
    Elem read_data;
    if (mem.read_data.try_read(read_data)) {
      ++i_read_resp;
    }
  }
}

void WriteOnlyBenchmark(tapa::async_mmap<Elem> mem, int64_t n, int32_t burst,
                        RandAddr::Addr id) {
  RandAddr write_addr(burst, id, id);

spin:
  for (int64_t i_write_req = 0, i_write_resp = 0; i_write_resp < n;) {
#pragma HLS pipeline II = 1

    // send write request
    if (i_write_req < n && !mem.write_addr.full() && !mem.write_data.full()) {
      mem.write_addr.try_write(write_addr++);
      mem.write_data.try_write(Elem(-1));
      ++i_write_req;
    }

    // collect write response
    uint8_t write_resp;
    if (mem.write_resp.try_read(write_resp)) {
      i_write_resp += write_resp + 1;
    }
  }
}

void ReadWriteBenchmark(tapa::async_mmap<Elem> mem, int64_t n, int32_t burst,
                        RandAddr::Addr id) {
  RandAddr read_addr(burst, id, id);
  RandAddr write_addr(burst, id, id);

spin:
  for (int64_t i_read_req = 0, i_write_resp = 0; i_write_resp < n;) {
#pragma HLS pipeline II = 1

    // send read request
    if (i_read_req < n && !mem.read_addr.full()) {
      mem.read_addr.try_write(read_addr++);
      ++i_read_req;
    }

    // collect read response and send write request
    if (!mem.read_data.empty() && !mem.write_addr.full() &&
        !mem.write_data.full()) {
      mem.read_data.read(nullptr);
      mem.write_addr.try_write(write_addr++);
      mem.write_data.try_write(Elem(-1));
    }

    // collect write response
    uint8_t write_resp;
    if (mem.write_resp.try_read(write_resp)) {
      i_write_resp += write_resp + 1;
    }
  }
}

void Wrapper(tapa::async_mmap<Elem> mem, int64_t n, int32_t burst,
             RandAddr::Addr id, bool is_read_enabled, bool is_write_enabled) {
#pragma HLS inline region
  id ^= RandAddr::Addr(RandAddr::kDefaultInitLfsr);
  if (is_read_enabled) {
    if (is_write_enabled) {
      ReadWriteBenchmark(mem, n, burst, id);
    } else {
      ReadOnlyBenchmark(mem, n, burst, id);
    }
  } else if (is_write_enabled) {
    WriteOnlyBenchmark(mem, n, burst, id);
  }
}

void bw(tapa::mmap<Elem> mem, int64_t n, int32_t burst, bool is_read_enabled,
        bool is_write_enabled) {
  tapa::task().invoke<tapa::join, CONCURRENCY>(
      Wrapper, mem, n, burst, tapa::seq(), is_read_enabled, is_write_enabled);
}
