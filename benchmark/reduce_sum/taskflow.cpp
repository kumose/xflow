#include "reduce_sum.hpp"
#include <xflow/taskflow.hpp>
#include <xflow/algorithm/reduce.hpp>

void reduce_sum_taskflow(unsigned num_threads) {

  static xf::Executor executor(num_threads);
  xf::Taskflow taskflow;

  double result = 0.0;

  taskflow.reduce_by_index(
    xf::IndexRange<size_t>(0, vec.size(), 1),
    result,
    [&](xf::IndexRange<size_t> range, std::optional<double> running_total) {
      double partial_sum = running_total ? *running_total : 0.0;
      for(size_t i=range.begin(); i<range.end(); i+=range.step_size()) {
        partial_sum += vec[i];
      }
      return partial_sum;
    },
    std::plus<double>()
  );

  executor.run(taskflow).get();

}

std::chrono::microseconds measure_time_taskflow(unsigned num_threads) {
  auto beg = std::chrono::high_resolution_clock::now();
  reduce_sum_taskflow(num_threads);
  auto end = std::chrono::high_resolution_clock::now();
  return std::chrono::duration_cast<std::chrono::microseconds>(end - beg);
}


