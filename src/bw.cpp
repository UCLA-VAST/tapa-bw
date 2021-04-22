#include <ap_int.h>

#include <tapa.h>

#include "bw.h"

void bw_kernel(tapa::async_mmap<Elem> mem, int64_t n, int32_t burst,
               RandAddr::Addr id, bool is_read_enabled, bool is_write_enabled) {
  id ^= RandAddr::Addr(RandAddr::kDefaultInitLfsr);
  RandAddr read_addr(burst, id, id);
  RandAddr write_addr(burst, id, id);

spin:
  for (int64_t i_read_req = 0, i_read_resp = 0,  // for read
       i_write_req = 0, i_write_resp = 0;        // for write
       (is_write_enabled ? i_write_resp : i_read_resp) < n;) {
#pragma HLS pipeline II = 1

    // send read request
    if (is_read_enabled && i_read_req < n && !mem.read_addr.full()) {
      mem.read_addr.try_write(read_addr++);
      ++i_read_req;
    }

    // send write request
    if (is_write_enabled && i_write_req < (is_read_enabled ? i_read_resp : n) &&
        !mem.write_addr.full() && !mem.write_data.full()) {
      mem.write_addr.try_write(write_addr++);
      mem.write_data.try_write(Elem(-1));
      ++i_write_req;
    }

    // collect read response
    Elem read_data;
    if (mem.read_data.try_read(read_data)) {
      ++i_read_resp;
    }

    // collect write response
    uint8_t write_resp;
    if (mem.write_resp.try_read(write_resp)) {
      i_write_resp += write_resp + 1;
    }
  }
}

void bw(tapa::mmap<Elem> mem, int64_t n, int32_t burst, bool is_read_enabled,
        bool is_write_enabled) {
  tapa::task().invoke<tapa::join, CONCURRENCY>(
      bw_kernel, mem, n, burst, tapa::seq(), is_read_enabled, is_write_enabled);
}
