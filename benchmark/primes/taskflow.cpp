#include "primes.hpp"
#include <xflow/taskflow.hpp>
#include <ranges>
#include <xflow/algorithm/reduce.hpp>

size_t primes_taskflow(size_t num_threads, size_t value) {

  static xf::Executor executor(num_threads);

  xf::Taskflow taskflow;

  size_t sum = 0;

  taskflow.reduce_by_index(
    xf::IndexRange<size_t>(1, value, 1),
    sum,
    [](xf::IndexRange<size_t> subrange, std::optional<size_t> running_total){
      size_t residual = running_total ? *running_total : 0;
      for(size_t i=subrange.begin(); i<subrange.end(); i+=subrange.step_size()) {
        residual += is_prime(i);
      }
      return residual;
    },
    std::plus<size_t>{},
    xf::DynamicPartitioner{primes_chunk}
  );

  executor.run(taskflow).wait();
 
  return sum; 
}


std::chrono::microseconds measure_time_taskflow(size_t num_threads, size_t value) {
  auto beg = std::chrono::high_resolution_clock::now();
  primes_taskflow(num_threads, value);
  auto end = std::chrono::high_resolution_clock::now();
  return std::chrono::duration_cast<std::chrono::microseconds>(end - beg);
}


