// This example demonstrates how to use the corun method from a running worker
// of an executor to avoid deadlock.
#include <xflow/taskflow.hpp>

int main(){
  
  const size_t N = 100;
  const size_t T = 1000;
  
  // create an executor and a taskflow
  xf::Executor executor(2);
  xf::Taskflow taskflow;

  std::array<xf::Taskflow, N> taskflows;

  std::atomic<size_t> counter{0};
  
  for(size_t n=0; n<N; n++) {
    for(size_t i=0; i<T; i++) {
      taskflows[n].emplace([&](){ counter++; });
    }
    taskflow.emplace([&executor, &tf=taskflows[n]](){
      executor.corun(tf);
      //executor.run(tf).wait();  <-- can result in deadlock
    });
  }

  executor.run(taskflow).wait();

  return 0;
}
