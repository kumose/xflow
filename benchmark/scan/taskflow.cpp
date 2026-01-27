#include "scan.hpp"
#include <xflow/taskflow.hpp>
#include <xflow/algorithm/scan.hpp>

void scan_taskflow(size_t num_threads) {

  static xf::Executor executor(num_threads);
  xf::Taskflow taskflow;

  taskflow.inclusive_scan(
    input.begin(), input.end(), output.begin(), std::multiplies<int>{}
  );

  executor.run(taskflow).get();
}

std::chrono::microseconds measure_time_taskflow(size_t num_threads) {
  auto beg = std::chrono::high_resolution_clock::now();
  scan_taskflow(num_threads);
  auto end = std::chrono::high_resolution_clock::now();
  return std::chrono::duration_cast<std::chrono::microseconds>(end - beg);
}


