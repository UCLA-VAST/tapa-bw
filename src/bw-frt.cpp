#include <stdlib.h>

#include <algorithm>
#include <memory>
#include <string>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <gflags/gflags.h>

#include <frt.h>
#include <tapa.h>

#include "bw.h"

DEFINE_string(bitstream, "", "FPGA bistream");

void bw(tapa::mmap<Elem> mem, int64_t n, int32_t burst) {
  auto kernel_time_ns_raw =
      mmap(nullptr, sizeof(int64_t), PROT_READ | PROT_WRITE,
           MAP_SHARED | MAP_ANONYMOUS, /*fd=*/-1, /*offset=*/0);
  PCHECK(kernel_time_ns_raw != MAP_FAILED);
  auto deleter = [](int64_t* p) { PCHECK(munmap(p, sizeof(int64_t)) == 0); };
  std::unique_ptr<int64_t, decltype(deleter)> kernel_time_ns(
      reinterpret_cast<int64_t*>(kernel_time_ns_raw), deleter);
  if (pid_t pid = fork()) {
    // Parent.
    PCHECK(pid != -1);
    int status = 0;
    CHECK_EQ(wait(&status), pid);
    CHECK(WIFEXITED(status));
    CHECK_EQ(WEXITSTATUS(status), EXIT_SUCCESS);

    setenv("KERNEL_TIME_NS", std::to_string(*kernel_time_ns).c_str(),
           /*replace=*/1);
    return;
  }

  // Child.
  {
    auto instance = fpga::Invoke(
        FLAGS_bitstream, fpga::Placeholder(mem.get(), mem.size()), n, burst);

    *kernel_time_ns = instance.ComputeTimeNanoSeconds();
  }
  exit(EXIT_SUCCESS);
}
