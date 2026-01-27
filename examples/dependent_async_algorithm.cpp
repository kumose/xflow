/**
  This program demonstrates how to use dependent async tasks to create
  algorithm tasks.
*/

#include <xflow/taskflow.hpp>
#include <xflow/algorithm/for_each.hpp>
#include <xflow/algorithm/transform.hpp>
#include <xflow/algorithm/reduce.hpp>

int main(){

  const size_t N = 65536;

  xf::Executor executor;
  
  int sum{1};
  std::vector<int> data(N);

  // for-each
  xf::AsyncTask A = executor.silent_dependent_async(xf::make_for_each_task(
    data.begin(), data.end(), [](int& i){ i = 1; }
  ));

  // transform
  xf::AsyncTask B = executor.silent_dependent_async(xf::make_transform_task(
    data.begin(), data.end(), data.begin(), [](int& i) { return i*2; }
  ), A);

  // reduce
  xf::AsyncTask C = executor.silent_dependent_async(xf::make_reduce_task(
    data.begin(), data.end(), sum, std::plus<int>{}
  ), B);
  
  // wait for all async task to complete
  executor.wait_for_all();
  
  // verify the result
  if(sum != N*2 + 1) {
    throw std::runtime_error("INCORRECT RESULT");
  }
  else {
    std::cout << "CORRECT RESULT\n";
  }
  
  return 0;
}




