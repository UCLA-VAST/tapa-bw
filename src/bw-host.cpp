
#include <numeric>
#include <vector>

#include <gflags/gflags.h>
#include <tapa.h>

#include "bw.h"

template <typename T>
using aligned_vector = std::vector<T, tapa::aligned_allocator<T>>;
using std::for_each;
using std::vector;

DEFINE_int32(logn, 6, "log2(number of requests)");
DEFINE_int32(burst, 1, "burst length");
DEFINE_string(bitstream, "", "bitstream");
DEFINE_bool(read, true, "enable memory read");
DEFINE_bool(write, true, "enable memory write");

void bw(tapa::mmap<Elem> mem, int64_t n, int32_t burst, bool is_read_enabled,
        bool is_write_enabled);

int main(int argc, char* argv[]) {
  FLAGS_logtostderr = true;
  FLAGS_colorlogtostderr = true;
  gflags::SetUsageMessage("TAPA Bandwidth Benchmark");
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  CHECK_GT(FLAGS_burst, 0);
  CHECK(FLAGS_read || FLAGS_write);
  const int64_t concurrency = CONCURRENCY * (FLAGS_read && FLAGS_write ? 2 : 1);

  aligned_vector<Elem> mem((1 << RandAddr::kWidth) + FLAGS_burst);

  vector<double> x, y;

  for (int64_t logn = FLAGS_logn; logn < FLAGS_logn / 2 * 3; ++logn) {
    const auto elapsed_time_ns = tapa::invoke_in_new_process(
        bw, FLAGS_bitstream, tapa::read_write_mmap<Elem>(mem),
        int64_t(1 << logn), FLAGS_burst - 1, FLAGS_read, FLAGS_write);
    x.push_back(elapsed_time_ns);
    y.push_back(1 << logn);

    if (FLAGS_write) {
      for (RandAddr::Addr id = 0; id < CONCURRENCY; ++id) {
        const auto init = id ^ RandAddr::Addr(RandAddr::kDefaultInitLfsr);
        RandAddr addr(FLAGS_burst - 1, init, init);

        for (int64_t i = 0; i < int64_t(1 << logn); ++i) {
          CHECK_EQ(mem[addr++], -1);
        }
      }
    }
  }

  if (const auto n = x.size(); n > 0) {
    // k = Σ(yi – y)(xi – x) / Σ(xi – x)²
    const auto m_x = std::accumulate(x.begin(), x.end(), 0.) / n;
    const auto m_y = std::accumulate(y.begin(), y.end(), 0.) / n;
    double s_xx = 0., s_xy = 0.;
    for (size_t i = 0; i < n; ++i) {
      const auto d_x = x[i] - m_x;
      s_xx += d_x * d_x;
      s_xy += d_x * (y[i] - m_y);
    }
    const auto k = s_xy / s_xx;
    // b = y - k * x
    const auto b = m_y - k * m_x;
    // ŷi = k * xi + b
    // δ = √(Σ(yi – ŷi)² / (n – 2) / Σ(xi – x)²)
    double s_yy = 0.f;
    for (size_t i = 0; i < n; ++i) {
      const auto d_y = k * x[i] + b - y[i];
      s_yy += d_y * d_y;
    }
    const auto delta = sqrt(s_yy / (n - 2) / s_xx);
    printf("%.3f ± %.3f MT/s\n", k * 1e3 * concurrency,
           delta * 1e3 * concurrency);
  }

  return 0;
}
