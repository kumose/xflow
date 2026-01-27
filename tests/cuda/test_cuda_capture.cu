#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest.h>
#include <xflow/taskflow.hpp>
#include <xflow/cuda/cudaflow.hpp>

void __global__ testKernel() {}

TEST_CASE("cudaFlowCapturer.noEventError") {
  xf::cudaFlow f;
  f.capture([](xf::cudaFlowCapturer& cpt) {
    cpt.on([] (cudaStream_t stream) {
      testKernel<<<256,256,0,stream>>>();
    });
    REQUIRE((cudaGetLastError() == cudaSuccess));
  });
  REQUIRE((cudaGetLastError() == cudaSuccess));
}
