// This program demonstrates the exception in subflow.

#include <xflow/taskflow.hpp>

int main() {

  xf::Executor executor;
  xf::Taskflow taskflow;

  taskflow.emplace([](xf::Subflow& sf) {
    xf::Task A = sf.emplace([]() {
      std::cout << "Task A\n";
      throw std::runtime_error("exception on A"); 
    });
    xf::Task B = sf.emplace([]() {
      std::cout << "Task B\n"; 
    });
    A.precede(B);
    sf.join();
  });

  try
  {
    executor.run(taskflow).get();
  }
  catch (const std::runtime_error& re)
  {
    std::cout << "exception thrown from running the taskflow: " << re.what() << '\n';
  }

  return 0;
}
