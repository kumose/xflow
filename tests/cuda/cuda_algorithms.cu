#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest.h>
#include <xflow/taskflow.hpp>
#include <xflow/cuda/cudaflow.hpp>
#include <xflow/cuda/algorithm/for_each.hpp>
#include <xflow/cuda/algorithm/transform.hpp>
#include <xflow/cuda/algorithm/reduce.hpp>
#include <xflow/cuda/algorithm/sort.hpp>
#include <xflow/cuda/algorithm/find.hpp>
#include <xflow/cuda/algorithm/scan.hpp>

constexpr float eps = 0.0001f;

template <typename T>
void run_and_wait(T& cf) {
  xf::cudaStream stream;
  cf.run(stream);
  stream.synchronize();
}

// --------------------------------------------------------
// Testcase: add2
// --------------------------------------------------------
template <typename T, typename F>
void add2() {

  //const unsigned N = 1<<20;
    
  xf::Taskflow taskflow;
  xf::Executor executor;

  for(size_t N=1; N<=(1<<20); N <<= 1) {

    taskflow.clear();

    T v1 = ::rand() % 100;
    T v2 = ::rand() % 100;

    std::vector<T> hx, hy;

    T* dx {nullptr};
    T* dy {nullptr};
    
    // allocate x
    auto allocate_x = taskflow.emplace([&]() {
      hx.resize(N, v1);
      REQUIRE(cudaMalloc(&dx, N*sizeof(T)) == cudaSuccess);
    }).name("allocate_x");

    // allocate y
    auto allocate_y = taskflow.emplace([&]() {
      hy.resize(N, v2);
      REQUIRE(cudaMalloc(&dy, N*sizeof(T)) == cudaSuccess);
    }).name("allocate_y");
    
    // axpy
    auto cudaflow = taskflow.emplace([&]() {
      F cf;
      auto h2d_x = cf.copy(dx, hx.data(), N).name("h2d_x");
      auto h2d_y = cf.copy(dy, hy.data(), N).name("h2d_y");
      auto d2h_x = cf.copy(hx.data(), dx, N).name("d2h_x");
      auto d2h_y = cf.copy(hy.data(), dy, N).name("d2h_y");
      //auto kernel = cf.add(dx, N, dx, dy);
      auto kernel = cf.transform(dx, dx+N, dy, 
        [] __device__ (T x) { return x + 2;  }
      );
      kernel.succeed(h2d_x, h2d_y)
            .precede(d2h_x, d2h_y);

      run_and_wait(cf);
    }).name("saxpy");

    cudaflow.succeed(allocate_x, allocate_y);

    // Add a verification task
    auto verifier = taskflow.emplace([&](){
      for (size_t i = 0; i < N; i++) {
        REQUIRE(std::fabs(hx[i] - v1) < eps);
        REQUIRE(std::fabs(hy[i] - (hx[i] + 2)) < eps);
      }
    }).succeed(cudaflow).name("verify");

    // free memory
    auto deallocate_x = taskflow.emplace([&](){
      REQUIRE(cudaFree(dx) == cudaSuccess);
    }).name("deallocate_x");
    
    auto deallocate_y = taskflow.emplace([&](){
      REQUIRE(cudaFree(dy) == cudaSuccess);
    }).name("deallocate_y");

    verifier.precede(deallocate_x, deallocate_y);

    executor.run(taskflow).wait();

    // standalone tramsform
    xf::cudaDefaultExecutionPolicy p;

    auto input  = xf::cuda_malloc_shared<T>(N);
    auto output = xf::cuda_malloc_shared<T>(N);
    for(size_t n=0; n<N; n++) {
      input[n] = 1;
    }

    xf::cuda_transform(p, input, input + N, output,
      [] __device__ (T i) { return i+2; }
    );
    cudaStreamSynchronize(0);

    for(size_t n=0; n<N; n++) {
      REQUIRE(output[n] == 3);
    }
  }
}

TEST_CASE("add2.int" * doctest::timeout(300)) {
  add2<int, xf::cudaFlow>();
}

TEST_CASE("add2.float" * doctest::timeout(300)) {
  add2<float, xf::cudaFlow>();
}

TEST_CASE("add2.double" * doctest::timeout(300)) {
  add2<double, xf::cudaFlow>();
}

TEST_CASE("capture_add2.int" * doctest::timeout(300)) {
  add2<int, xf::cudaFlowCapturer>();
}

TEST_CASE("capture_add2.float" * doctest::timeout(300)) {
  add2<float, xf::cudaFlowCapturer>();
}

TEST_CASE("capture_add2.double" * doctest::timeout(300)) {
  add2<double, xf::cudaFlowCapturer>();
}

// ----------------------------------------------------------------------------
// transform
// ----------------------------------------------------------------------------

template <typename F>
void transform() {
  
  F cudaflow;
    
  for(unsigned n=1; n<=1234567; n = n*2 + 1) {

    cudaflow.clear();

    auto src1 = xf::cuda_malloc_shared<int>(n);
    auto src2 = xf::cuda_malloc_shared<int>(n);
    auto dest = xf::cuda_malloc_shared<int>(n);

    for(unsigned i=0; i<n; i++) {
      src1[i] = 10;
      src2[i] = 90;
      dest[i] = 0;
    }

    cudaflow.transform(src1, src1+n, src2, dest,
      []__device__(int s1, int s2) { return s1 + s2; } 
    );

    run_and_wait(cudaflow);

    for(unsigned i=0; i<n; i++){
      REQUIRE(dest[i] == src1[i] + src2[i]);
    }
  }
}

TEST_CASE("cudaflow.transform" * doctest::timeout(300)) {
  transform<xf::cudaFlow>();
}

TEST_CASE("capture.transform" * doctest::timeout(300) ) {
  transform<xf::cudaFlowCapturer>();
}

// ----------------------------------------------------------------------------
// reduce
// ----------------------------------------------------------------------------

template <typename T, typename F>
void reduce() {

  for(int n=1; n<=1234567; n = n*2 + 1) {

    xf::Taskflow taskflow;
    xf::Executor executor;

    T sum = 0;

    std::vector<T> cpu(n);
    for(auto& i : cpu) {
      i = ::rand()%100-50;
      sum += i;
    }

    T sol;
    
    T* gpu = nullptr;
    T* res = nullptr;

    auto cputask = taskflow.emplace([&](){
      REQUIRE(cudaMalloc(&gpu, n*sizeof(T)) == cudaSuccess);
      REQUIRE(cudaMalloc(&res, 1*sizeof(T)) == cudaSuccess);
    });

    xf::Task gputask;
    
    gputask = taskflow.emplace([&]() {
      F cf;
      auto d2h = cf.copy(&sol, res, 1);
      auto h2d = cf.copy(gpu, cpu.data(), n);
      auto set = cf.single_task([res] __device__ () mutable {
        *res = 1000;
      });
      auto kernel = cf.reduce(
        gpu, gpu+n, res, [] __device__ (T a, T b) mutable { 
          return a + b;
        }
      );
      kernel.succeed(h2d, set);
      d2h.succeed(kernel);
      run_and_wait(cf);
    });

    cputask.precede(gputask);
    
    executor.run(taskflow).wait();

    REQUIRE(std::fabs(sum-sol+1000) < 0.0001);

    REQUIRE(cudaFree(gpu) == cudaSuccess);
    REQUIRE(cudaFree(res) == cudaSuccess);
  }
}

TEST_CASE("cudaflow.reduce.int" * doctest::timeout(300)) {
  reduce<int, xf::cudaFlow>();
}

TEST_CASE("cudaflow.reduce.float" * doctest::timeout(300)) {
  reduce<float, xf::cudaFlow>();
}

TEST_CASE("cudaflow.reduce.double" * doctest::timeout(300)) {
  reduce<double, xf::cudaFlow>();
}

TEST_CASE("capture.reduce.int" * doctest::timeout(300)) {
  reduce<int, xf::cudaFlowCapturer>();
}

TEST_CASE("capture.reduce.float" * doctest::timeout(300)) {
  reduce<float, xf::cudaFlowCapturer>();
}

TEST_CASE("capture.reduce.double" * doctest::timeout(300)) {
  reduce<double, xf::cudaFlow>();
}

// ----------------------------------------------------------------------------
// uninitialized_reduce
// ----------------------------------------------------------------------------

template <typename T, typename F>
void uninitialized_reduce() {

  for(int n=1; n<=1234567; n = n*2 + 1) {

    xf::Taskflow taskflow;
    xf::Executor executor;

    T sum = 0;

    std::vector<T> cpu(n);
    for(auto& i : cpu) {
      i = ::rand()%100-50;
      sum += i;
    }

    T sol;
    
    T* gpu = nullptr;
    T* res = nullptr;

    auto cputask = taskflow.emplace([&](){
      REQUIRE(cudaMalloc(&gpu, n*sizeof(T)) == cudaSuccess);
      REQUIRE(cudaMalloc(&res, 1*sizeof(T)) == cudaSuccess);
    });

    xf::Task gputask;
    
    gputask = taskflow.emplace([&]() {
      F cf;
      auto d2h = cf.copy(&sol, res, 1);
      auto h2d = cf.copy(gpu, cpu.data(), n);
      auto set = cf.single_task([res] __device__ () mutable {
        *res = 1000;
      });
      auto kernel = cf.uninitialized_reduce(
        gpu, gpu+n, res, [] __device__ (T a, T b) { 
          return a + b;
        }
      );
      kernel.succeed(h2d, set);
      d2h.succeed(kernel);
      run_and_wait(cf);
    });

    cputask.precede(gputask);
    
    executor.run(taskflow).wait();

    REQUIRE(std::fabs(sum-sol) < 0.0001);

    REQUIRE(cudaFree(gpu) == cudaSuccess);
    REQUIRE(cudaFree(res) == cudaSuccess);
  }
}

TEST_CASE("cudaflow.uninitialized_reduce.int" * doctest::timeout(300)) {
  uninitialized_reduce<int, xf::cudaFlow>();
}

TEST_CASE("cudaflow.uninitialized_reduce.float" * doctest::timeout(300)) {
  uninitialized_reduce<float, xf::cudaFlow>();
}

TEST_CASE("cudaflow.uninitialized_reduce.double" * doctest::timeout(300)) {
  uninitialized_reduce<double, xf::cudaFlow>();
}

TEST_CASE("capture.uninitialized_reduce.int" * doctest::timeout(300)) {
  uninitialized_reduce<int, xf::cudaFlowCapturer>();
}

TEST_CASE("capture.uninitialized_reduce.float" * doctest::timeout(300)) {
  uninitialized_reduce<float, xf::cudaFlowCapturer>();
}

TEST_CASE("capture.uninitialized_reduce.double" * doctest::timeout(300)) {
  uninitialized_reduce<double, xf::cudaFlow>();
}

// ----------------------------------------------------------------------------
// transform_reduce
// ----------------------------------------------------------------------------

template <typename T, typename F>
void transform_reduce() {
    
  xf::Executor executor;

  for(int n=1; n<=1234567; n = n*2 + 1) {

    xf::Taskflow taskflow;

    T sum = 0;

    std::vector<T> cpu(n);
    for(auto& i : cpu) {
      i = ::rand()%100-50;
      sum += i;
    }

    T sol;
    
    T* gpu = nullptr;
    T* res = nullptr;

    auto cputask = taskflow.emplace([&](){
      REQUIRE(cudaMalloc(&gpu, n*sizeof(T)) == cudaSuccess);
      REQUIRE(cudaMalloc(&res, 1*sizeof(T)) == cudaSuccess);
    });

    xf::Task gputask;
    
    gputask = taskflow.emplace([&]() {
      F cf;
      auto d2h = cf.copy(&sol, res, 1);
      auto h2d = cf.copy(gpu, cpu.data(), n);
      auto set = cf.single_task([res] __device__ () mutable {
        *res = 1000;
      });
      auto kernel = cf.transform_reduce(
        gpu, gpu+n, res, 
        [] __device__ (T a, T b) { return a + b; },
        [] __device__ (T a) { return a + 1; }
      );
      kernel.succeed(h2d, set);
      d2h.succeed(kernel);
      run_and_wait(cf);
    });

    cputask.precede(gputask);
    
    executor.run(taskflow).wait();

    REQUIRE(std::fabs(sum+n+1000-sol) < 0.0001);

    REQUIRE(cudaFree(gpu) == cudaSuccess);
    REQUIRE(cudaFree(res) == cudaSuccess);
  }
}

TEST_CASE("cudaflow.transform_reduce.int" * doctest::timeout(300)) {
  transform_reduce<int, xf::cudaFlow>();
}

TEST_CASE("cudaflow.transform_reduce.float" * doctest::timeout(300)) {
  transform_reduce<float, xf::cudaFlow>();
}

TEST_CASE("cudaflow.transform_reduce.double" * doctest::timeout(300)) {
  transform_reduce<double, xf::cudaFlow>();
}

TEST_CASE("capture.transform_reduce.int" * doctest::timeout(300)) {
  transform_reduce<int, xf::cudaFlowCapturer>();
}

TEST_CASE("capture.transform_reduce.float" * doctest::timeout(300)) {
  transform_reduce<float, xf::cudaFlowCapturer>();
}

TEST_CASE("capture.transform_reduce.double" * doctest::timeout(300)) {
  transform_reduce<double, xf::cudaFlowCapturer>();
}

// ----------------------------------------------------------------------------
// transform_uninitialized_reduce
// ----------------------------------------------------------------------------

template <typename T, typename F>
void transform_uninitialized_reduce() {
    
  xf::Executor executor;

  for(int n=1; n<=1234567; n = n*2 + 1) {

    xf::Taskflow taskflow;

    T sum = 0;

    std::vector<T> cpu(n);
    for(auto& i : cpu) {
      i = ::rand()%100-50;
      sum += i;
    }

    T sol;
    
    T* gpu = nullptr;
    T* res = nullptr;

    auto cputask = taskflow.emplace([&](){
      REQUIRE(cudaMalloc(&gpu, n*sizeof(T)) == cudaSuccess);
      REQUIRE(cudaMalloc(&res, 1*sizeof(T)) == cudaSuccess);
    });

    xf::Task gputask;
    
    gputask = taskflow.emplace([&]() {
      F cf;
      auto d2h = cf.copy(&sol, res, 1);
      auto h2d = cf.copy(gpu, cpu.data(), n);
      auto set = cf.single_task([res] __device__ () mutable {
        *res = 1000;
      });
      auto kernel = cf.transform_uninitialized_reduce(
        gpu, gpu+n, res, 
        [] __device__ (T a, T b) { return a + b; },
        [] __device__ (T a) { return a + 1; }
      );
      kernel.succeed(h2d, set);
      d2h.succeed(kernel);
      run_and_wait(cf);
    });

    cputask.precede(gputask);
    
    executor.run(taskflow).wait();

    REQUIRE(std::fabs(sum+n-sol) < 0.0001);

    REQUIRE(cudaFree(gpu) == cudaSuccess);
    REQUIRE(cudaFree(res) == cudaSuccess);
  }
}

TEST_CASE("cudaflow.transform_uninitialized_reduce.int" * doctest::timeout(300)) {
  transform_uninitialized_reduce<int, xf::cudaFlow>();
}

TEST_CASE("cudaflow.transform_uninitialized_reduce.float" * doctest::timeout(300)) {
  transform_uninitialized_reduce<float, xf::cudaFlow>();
}

TEST_CASE("cudaflow.transform_uninitialized_reduce.double" * doctest::timeout(300)) {
  transform_uninitialized_reduce<double, xf::cudaFlow>();
}

TEST_CASE("capture.transform_uninitialized_reduce.int" * doctest::timeout(300)) {
  transform_uninitialized_reduce<int, xf::cudaFlowCapturer>();
}

TEST_CASE("capture.transform_uninitialized_reduce.float" * doctest::timeout(300)) {
  transform_uninitialized_reduce<float, xf::cudaFlowCapturer>();
}

TEST_CASE("capture.transform_uninitialized_reduce.double" * doctest::timeout(300)) {
  transform_uninitialized_reduce<double, xf::cudaFlowCapturer>();
}

// ----------------------------------------------------------------------------
// scan
// ----------------------------------------------------------------------------

template <typename T, typename F>
void scan() {
    
  xf::Executor executor;
  xf::Taskflow taskflow;

  for(int N=1; N<=1234567; N = N*2 + 1) {

    taskflow.clear();
  
    auto data1 = xf::cuda_malloc_shared<T>(N);
    auto data2 = xf::cuda_malloc_shared<T>(N);
    auto scan1 = xf::cuda_malloc_shared<T>(N);
    auto scan2 = xf::cuda_malloc_shared<T>(N);

    // initialize the data
    for(int i=0; i<N; i++) {
      data1[i] = T(i);
      data2[i] = T(i);
      scan1[i] = 0;
      scan2[i] = 0;
    }
    
    // perform reduction
    taskflow.emplace([&](){
      F cudaflow;
      // inclusive scan
      cudaflow.inclusive_scan(
        data1, data1+N, scan1, [] __device__ (T a, T b){ return a+b; }
      );
      // exclusive scan
      cudaflow.exclusive_scan(
        data2, data2+N, scan2, [] __device__ (T a, T b){ return a+b; }
      );
      run_and_wait(cudaflow);
    });

    executor.run(taskflow).wait();
    
    // inspect 
    for(int i=1; i<N; i++) {
      REQUIRE(scan1[i] == (scan1[i-1]+data1[i]));
      REQUIRE(scan2[i] == (scan2[i-1]+data2[i-1]));
    }

    // test standalone algorithms
    
    // initialize the data
    for(int i=0; i<N; i++) {
      data1[i] = T(i);
      data2[i] = T(i);
      scan1[i] = 0;
      scan2[i] = 0;
    }

    // allocate temporary buffer
    xf::cudaDeviceVector<std::byte> temp(
      xf::cuda_scan_buffer_size<xf::cudaDefaultExecutionPolicy, T>(N)
    );
      
    xf::cuda_inclusive_scan(xf::cudaDefaultExecutionPolicy{},
      data1, data1+N, scan1, xf::cuda_plus<T>{}, temp.data()
    );
    cudaStreamSynchronize(0);

    xf::cuda_exclusive_scan(xf::cudaDefaultExecutionPolicy{},
      data2, data2+N, scan2, xf::cuda_plus<T>{}, temp.data()
    );
    cudaStreamSynchronize(0);
    
    // inspect 
    for(int i=1; i<N; i++) {
      REQUIRE(scan1[i] == (scan1[i-1]+data1[i]));
      REQUIRE(scan2[i] == (scan2[i-1]+data2[i-1]));
    }

    REQUIRE(cudaFree(data1) == cudaSuccess);
    REQUIRE(cudaFree(data2) == cudaSuccess);
    REQUIRE(cudaFree(scan1) == cudaSuccess);
    REQUIRE(cudaFree(scan2) == cudaSuccess);
  }
}

TEST_CASE("cudaflow.scan.int" * doctest::timeout(300)) {
  scan<int, xf::cudaFlow>();
}

TEST_CASE("cudaflow.scan.size_t" * doctest::timeout(300)) {
  scan<size_t, xf::cudaFlow>();
}

TEST_CASE("capture.scan.int" * doctest::timeout(300)) {
  scan<int, xf::cudaFlowCapturer>();
}

TEST_CASE("capture.scan.size_t" * doctest::timeout(300)) {
  scan<size_t, xf::cudaFlowCapturer>();
}

// ----------------------------------------------------------------------------
// transofrm scan
// ----------------------------------------------------------------------------

template <typename T, typename F>
void transform_scan() {
    
  xf::Executor executor;
  xf::Taskflow taskflow;

  for(int N=1; N<=1234567; N = N*2 + 1) {

    taskflow.clear();
  
    auto data1 = xf::cuda_malloc_shared<T>(N);
    auto data2 = xf::cuda_malloc_shared<T>(N);
    auto scan1 = xf::cuda_malloc_shared<T>(N);
    auto scan2 = xf::cuda_malloc_shared<T>(N);

    // initialize the data
    for(int i=0; i<N; i++) {
      data1[i] = T(i);
      data2[i] = T(i);
      scan1[i] = 0;
      scan2[i] = 0;
    }
    
    // perform reduction
    taskflow.emplace([&](){

      F cudaflow;

      // inclusive scan
      cudaflow.transform_inclusive_scan(
        data1, data1+N, scan1, 
        [] __device__ (T a, T b){ return a+b; },
        [] __device__ (T a) { return a*10; }
      );
      // exclusive scan
      cudaflow.transform_exclusive_scan(
        data2, data2+N, scan2, 
        [] __device__ (T a, T b){ return a+b; },
        [] __device__ (T a) { return a*10; }
      );

      run_and_wait(cudaflow);
    });

    executor.run(taskflow).wait();
    
    // standalone algorithms
    
    // initialize the data
    for(int i=0; i<N; i++) {
      data1[i] = T(i);
      data2[i] = T(i);
      scan1[i] = 0;
      scan2[i] = 0;
    }
    
    // allocate temporary buffer
    xf::cudaDeviceVector<std::byte> temp(
      xf::cuda_scan_buffer_size<xf::cudaDefaultExecutionPolicy, T>(N)
    );
      
    xf::cuda_transform_inclusive_scan(xf::cudaDefaultExecutionPolicy{},
      data1, data1+N, scan1, 
      [] __device__ (T a, T b){ return a+b; },
      [] __device__ (T a) { return a*10; },
      temp.data()
    );
    cudaStreamSynchronize(0);
      
    xf::cuda_transform_exclusive_scan(xf::cudaDefaultExecutionPolicy{},
      data2, data2+N, scan2, 
      [] __device__ (T a, T b){ return a+b; },
      [] __device__ (T a) { return a*10; },
      temp.data()
    );
    cudaStreamSynchronize(0);
    
    // inspect 
    for(int i=1; i<N; i++) {
      REQUIRE(scan1[i] == (scan1[i-1]+data1[i]*10));
      REQUIRE(scan2[i] == (scan2[i-1]+data2[i-1]*10));
    }

    REQUIRE(cudaFree(data1) == cudaSuccess);
    REQUIRE(cudaFree(data2) == cudaSuccess);
    REQUIRE(cudaFree(scan1) == cudaSuccess);
    REQUIRE(cudaFree(scan2) == cudaSuccess);
  }
}

TEST_CASE("cudaflow.scan.int" * doctest::timeout(300)) {
  transform_scan<int, xf::cudaFlow>();
}

TEST_CASE("cudaflow.scan.size_t" * doctest::timeout(300)) {
  transform_scan<size_t, xf::cudaFlow>();
}

TEST_CASE("capture.transform_scan.int" * doctest::timeout(300)) {
  transform_scan<int, xf::cudaFlowCapturer>();
}

TEST_CASE("capture.transform_scan.size_t" * doctest::timeout(300)) {
  transform_scan<size_t, xf::cudaFlowCapturer>();
}

// ----------------------------------------------------------------------------
// merge
// ----------------------------------------------------------------------------

template <typename T, typename F>
void merge_keys() {
    
  xf::Executor executor;
  xf::Taskflow taskflow;

  for(int N=0; N<=1234567; N = N*2 + 1) {

    taskflow.clear();

    auto a = xf::cuda_malloc_shared<T>(N);
    auto b = xf::cuda_malloc_shared<T>(N);
    auto c = xf::cuda_malloc_shared<T>(2*N);
  
    xf::cudaStream s;
    auto p = xf::cudaDefaultExecutionPolicy{s};

    // ----------------- standalone algorithms

    // initialize the data
    for(int i=0; i<N; i++) {
      a[i] = T(rand()%100);
      b[i] = T(rand()%100);
    }

    std::sort(a, a+N);
    std::sort(b, b+N);
    
    auto bufsz = xf::cuda_merge_buffer_size<decltype(p)>(N, N);
    xf::cudaDeviceVector<std::byte> buf(bufsz);

    xf::cuda_merge(p, a, a+N, b, b+N, c, xf::cuda_less<T>{}, buf.data());
    s.synchronize();

    REQUIRE(std::is_sorted(c, c+2*N));
    
    /*// ----------------- cudaFlow capturer
    for(int i=0; i<N*2; i++) {
      c[i] = rand();      
    }
    
    taskflow.emplace([&](){
      F cudaflow;
      cudaflow.merge(a, a+N, b, b+N, c, xf::cuda_less<T>{});
      run_and_wait(cudaflow);
    });

    executor.run(taskflow).wait();
    
    REQUIRE(std::is_sorted(c, c+2*N));*/
    
    REQUIRE(cudaFree(a) == cudaSuccess);
    REQUIRE(cudaFree(b) == cudaSuccess);
    REQUIRE(cudaFree(c) == cudaSuccess);
  }
}

TEST_CASE("cudaflow.merge_keys.int" * doctest::timeout(300)) {
  merge_keys<int, xf::cudaFlow>();
}

TEST_CASE("cudaflow.merge_keys.float" * doctest::timeout(300)) {
  merge_keys<float, xf::cudaFlow>();
}

TEST_CASE("cudaflow.merge_keys.int" * doctest::timeout(300)) {
  merge_keys<int, xf::cudaFlow>();
}

TEST_CASE("capture.merge_keys.float" * doctest::timeout(300)) {
  merge_keys<float, xf::cudaFlowCapturer>();
}

// ----------------------------------------------------------------------------
// merge_by_keys
// ----------------------------------------------------------------------------

template <typename T, typename F>
void merge_keys_values() {
    
  xf::Executor executor;
  xf::Taskflow taskflow;

  for(int N=0; N<=1234567; N = N*2 + 1) {

    taskflow.clear();

    auto a_k = xf::cuda_malloc_shared<T>(N);
    auto b_k = xf::cuda_malloc_shared<T>(N);
    auto c_k = xf::cuda_malloc_shared<T>(2*N);
    auto a_v = xf::cuda_malloc_shared<int>(N);
    auto b_v = xf::cuda_malloc_shared<int>(N);
    auto c_v = xf::cuda_malloc_shared<int>(2*N);

    xf::cudaStream s;
    auto p = xf::cudaDefaultExecutionPolicy{s};

    // ----------------- standalone algorithms

    // initialize the data
    for(int i=0; i<N; i++) {
      a_k[i] =  (i*2+1);
      a_v[i] = -(i*2+1);
      b_k[i] =  (i+1)*2;
      b_v[i] = -(i+1)*2;
      c_k[i] = c_k[i+N] = c_v[i] = c_v[i+N] = 0;
    }

    auto bufsz = xf::cuda_merge_buffer_size<decltype(p)>(N, N);
    xf::cudaDeviceVector<std::byte> buf(bufsz);

    xf::cuda_merge_by_key(
      p, 
      a_k, a_k+N, a_v, 
      b_k, b_k+N, b_v,
      c_k, c_v,
      xf::cuda_less<T>{},
      buf.data()
    );
    s.synchronize();

    for(int i=0; i<2*N; i++) {
      REQUIRE(c_k[i] == (i+1));
      REQUIRE(c_v[i] == -(i+1));
    }
    /*
    // ----------------- cudaFlow capturer
    // initialize the data
    for(int i=0; i<N; i++) {
      a_k[i] =  (i*2+1);
      a_v[i] = -(i*2+1);
      b_k[i] =  (i+1)*2;
      b_v[i] = -(i+1)*2;
      c_k[i] = c_k[i+N] = c_v[i] = c_v[i+N] = 0;
    }
    
    taskflow.emplace([&](){
      F cudaflow;
      cudaflow.merge_by_key(
        a_k, a_k+N, a_v, b_k, b_k+N, b_v, c_k, c_v, xf::cuda_less<T>{}
      );
      run_and_wait(cudaflow);
    });

    executor.run(taskflow).wait();
    
    for(int i=0; i<2*N; i++) {
      REQUIRE(c_k[i] == (i+1));
      REQUIRE(c_v[i] == -(i+1));
    }*/
    
    REQUIRE(cudaFree(a_k) == cudaSuccess);
    REQUIRE(cudaFree(b_k) == cudaSuccess);
    REQUIRE(cudaFree(c_k) == cudaSuccess);
    REQUIRE(cudaFree(a_v) == cudaSuccess);
    REQUIRE(cudaFree(b_v) == cudaSuccess);
    REQUIRE(cudaFree(c_v) == cudaSuccess);
  }
}

TEST_CASE("cudaflow.merge_keys_values.int" * doctest::timeout(300)) {
  merge_keys_values<int, xf::cudaFlow>();
}

TEST_CASE("cudaflow.merge_keys_values.float" * doctest::timeout(300)) {
  merge_keys_values<float, xf::cudaFlow>();
}

TEST_CASE("capturer.merge_keys_values.int" * doctest::timeout(300)) {
  merge_keys_values<int, xf::cudaFlowCapturer>();
}

TEST_CASE("capturer.merge_keys_values.float" * doctest::timeout(300)) {
  merge_keys_values<float, xf::cudaFlowCapturer>();
}

// ----------------------------------------------------------------------------
// sort
// ----------------------------------------------------------------------------

template <typename T, typename F>
void sort_keys() {
    
  xf::Executor executor;
  xf::Taskflow taskflow;

  for(int N=0; N<=1234567; N = N*2 + 1) {

    taskflow.clear();

    auto a = xf::cuda_malloc_shared<T>(N);
    xf::cudaStream s;
    auto p = xf::cudaDefaultExecutionPolicy{s};

    // ----------------- standalone asynchronous algorithms

    // initialize the data
    for(int i=0; i<N; i++) {
      a[i] = T(rand()%1000);
    }

    auto bufsz = xf::cuda_sort_buffer_size<decltype(p), T>(N);
    xf::cudaDeviceVector<std::byte> buf(bufsz);
    xf::cuda_sort(p, a, a+N, xf::cuda_less<T>{}, buf.data());
    s.synchronize();
    REQUIRE(std::is_sorted(a, a+N));

    /*
    // ----------------- cudaflow capturer
    for(int i=0; i<N; i++) {
      a[i] = T(rand()%1000);
    }
    
    taskflow.emplace([&](){
      F cudaflow;
      cudaflow.sort(a, a+N, xf::cuda_less<T>{});
      run_and_wait(cudaflow);
    });

    executor.run(taskflow).wait();

    REQUIRE(std::is_sorted(a, a+N)); */
    
    REQUIRE(cudaFree(a) == cudaSuccess);
  }
}

TEST_CASE("cudaflow.sort_keys.int" * doctest::timeout(300)) {
  sort_keys<int, xf::cudaFlow>();
}

TEST_CASE("cudaflow.sort_keys.float" * doctest::timeout(300)) {
  sort_keys<float, xf::cudaFlow>();
}

TEST_CASE("capture.sort_keys.int" * doctest::timeout(300)) {
  sort_keys<int, xf::cudaFlowCapturer>();
}

TEST_CASE("capture.sort_keys.float" * doctest::timeout(300)) {
  sort_keys<float, xf::cudaFlowCapturer>();
}

// ----------------------------------------------------------------------------
// sort key-value
// ----------------------------------------------------------------------------

template <typename T, typename F>
void sort_keys_values() {

  std::random_device rd;
  std::mt19937 g(rd());
    
  xf::Executor executor;
  xf::Taskflow taskflow;

  for(int N=1; N<=1234567; N = N*2 + 1) {

    taskflow.clear();

    auto a = xf::cuda_malloc_shared<T>(N);
    auto b = xf::cuda_malloc_shared<int>(N);

    xf::cudaStream s;
    auto p = xf::cudaDefaultExecutionPolicy{s};

    std::vector<int> indices(N);

    // ----------------- standalone asynchronous algorithms

    // initialize the data
    for(int i=0; i<N; i++) {
      a[i] = i;
      b[i] = i;
      indices[i] = i;
      //printf("a[%d]=%d, b[%d]=%d\n", i, a[i], i, b[i]);
    }
    std::shuffle(a, a+N, g);

    std::sort(indices.begin(), indices.end(), [&](auto i, auto j){
      return a[i] < a[j];
    });

    auto bufsz = xf::cuda_sort_buffer_size<decltype(p), T, int>(N);
    xf::cudaDeviceVector<std::byte> buf(bufsz);
    xf::cuda_sort_by_key(p, a, a+N, b, xf::cuda_less<T>{}, buf.data());
    s.synchronize();

    REQUIRE(std::is_sorted(a, a+N));
    for(int i=0; i<N; i++) {
      REQUIRE(indices[i] == b[i]);
    }

    /*// ----------------- cudaflow capturer
    // initialize the data
    for(int i=0; i<N; i++) {
      b[i] = i;
      indices[i] = i;
      //printf("a[%d]=%d, b[%d]=%d\n", i, a[i], i, b[i]);
    }
    std::shuffle(a, a+N, g);

    std::sort(indices.begin(), indices.end(), [&](auto i, auto j){
      return a[i] > a[j];
    });

    taskflow.emplace([&](){
      F cudaflow;
      cudaflow.sort_by_key(a, a+N, b, xf::cuda_greater<T>{});
      run_and_wait(cudaflow);
    });

    executor.run(taskflow).wait();

    REQUIRE(std::is_sorted(a, a+N, std::greater<T>{}));
    for(int i=0; i<N; i++) {
      REQUIRE(indices[i] == b[i]);
    }*/
    
    REQUIRE(cudaFree(a) == cudaSuccess);
    REQUIRE(cudaFree(b) == cudaSuccess);
  }
}

TEST_CASE("cudaflow.sort_keys_values.int" * doctest::timeout(300)) {
  sort_keys_values<int, xf::cudaFlow>();
}

TEST_CASE("cudaflow.sort_keys_values.float" * doctest::timeout(300)) {
  sort_keys_values<float, xf::cudaFlow>();
}

TEST_CASE("capture.sort_keys_values.int" * doctest::timeout(300)) {
  sort_keys_values<int, xf::cudaFlowCapturer>();
}

TEST_CASE("capture.sort_keys_values.float" * doctest::timeout(300)) {
  sort_keys_values<float, xf::cudaFlowCapturer>();
}

// ----------------------------------------------------------------------------
// find-if
// ----------------------------------------------------------------------------

template <typename T, typename F>
void find_if() {
    
  xf::Executor executor;
  xf::Taskflow taskflow;

  for(int N=0; N<=1234567; N += std::max(N/100, 1)) {

    taskflow.clear();

    auto a = xf::cuda_malloc_shared<T>(N);
    auto r = xf::cuda_malloc_shared<unsigned>(1);

    xf::cudaStream s;
    auto p = xf::cudaDefaultExecutionPolicy{s};
    
    // initialize the data
    for(int i=0; i<N; i++) {
      a[i] = i;
    }
    *r = 1234;

    // ----------------- standalone asynchronous algorithms

    xf::cuda_find_if(p, a, a+N, r, []__device__(int v){ return v == 5000; });
    s.synchronize();

    if(N <= 5000) {
      REQUIRE(*r == N);
    }
    else {
      REQUIRE(*r == 5000);
    }

    // ----------------- cudaflow capturer
    *r = 1234;
    
    taskflow.emplace([&](){
      F cudaflow;
      cudaflow.find_if(a, a+N, r, []__device__(int v){ return v == 5000; });
      run_and_wait(cudaflow);
    });

    executor.run(taskflow).wait();
    
    if(N <= 5000) {
      REQUIRE(*r == N);
    }
    else {
      REQUIRE(*r == 5000);
    }
    
    REQUIRE(cudaFree(a) == cudaSuccess);
    REQUIRE(cudaFree(r) == cudaSuccess);
  }
}

TEST_CASE("cudaflow.find_if.int" * doctest::timeout(300)) {
  find_if<int, xf::cudaFlow>();
}

TEST_CASE("cudaflow.find_if.float" * doctest::timeout(300)) {
  find_if<float, xf::cudaFlow>();
}

TEST_CASE("capture.find_if.int" * doctest::timeout(300)) {
  find_if<int, xf::cudaFlowCapturer>();
}

TEST_CASE("capture.find_if.float" * doctest::timeout(300)) {
  find_if<float, xf::cudaFlowCapturer>();
}

// ----------------------------------------------------------------------------
// min_element
// ----------------------------------------------------------------------------

template <typename T, typename F>
void min_element() {
    
  xf::Executor executor;
  xf::Taskflow taskflow;

  for(int N=0; N<=1234567; N += std::max(N/10, 1)) {

    taskflow.clear();

    auto a = xf::cuda_malloc_shared<T>(N);
    auto r = xf::cuda_malloc_shared<unsigned>(1);
    auto min = std::numeric_limits<T>::max();
    
    xf::cudaStream s;
    auto p = xf::cudaDefaultExecutionPolicy{s};

    // initialize the data
    for(int i=0; i<N; i++) {
      a[i] = rand();
      min = std::min(min, a[i]);
    }
    *r = 1234;

    // ----------------- standalone asynchronous algorithms

    xf::cudaDeviceVector<std::byte> buf(
      xf::cuda_min_element_buffer_size<decltype(p), T>(N)
    );

    xf::cuda_min_element(
      p, a, a+N, r, xf::cuda_less<T>{}, buf.data()
    );
    s.synchronize();

    if(min != std::numeric_limits<T>::max()) {
      REQUIRE(a[*r] == min);
    }
    else {
      REQUIRE(*r == N);
    }
    
    // ----------------- cudaflow
    *r = 1234;
    
    taskflow.emplace([&](){
      F cudaflow;
      cudaflow.min_element(a, a+N, r, xf::cuda_less<T>{});
      run_and_wait(cudaflow);
    });

    executor.run(taskflow).wait();
    
    if(min != std::numeric_limits<T>::max()) {
      REQUIRE(a[*r] == min);
    }
    else {
      REQUIRE(*r == N);
    }
    
    REQUIRE(cudaFree(a) == cudaSuccess);
    REQUIRE(cudaFree(r) == cudaSuccess);
  }
}

TEST_CASE("cudaflow.min_element.int" * doctest::timeout(300)) {
  min_element<int, xf::cudaFlow>();
}

TEST_CASE("cudaflow.min_element.float" * doctest::timeout(300)) {
  min_element<float, xf::cudaFlow>();
}

TEST_CASE("capturer.min_element.int" * doctest::timeout(300)) {
  min_element<int, xf::cudaFlowCapturer>();
}

TEST_CASE("capturer.min_element.float" * doctest::timeout(300)) {
  min_element<float, xf::cudaFlowCapturer>();
}

// ----------------------------------------------------------------------------
// max_element
// ----------------------------------------------------------------------------

template <typename T, typename F>
void max_element() {
    
  xf::Executor executor;
  xf::Taskflow taskflow;

  for(int N=0; N<=1234567; N += std::max(N/10, 1)) {

    taskflow.clear();

    auto a = xf::cuda_malloc_shared<T>(N);
    auto r = xf::cuda_malloc_shared<unsigned>(1);
    auto max = std::numeric_limits<T>::lowest();
    
    xf::cudaStream s;
    auto p = xf::cudaDefaultExecutionPolicy{s};

    // initialize the data
    for(int i=0; i<N; i++) {
      a[i] = rand();
      max = std::max(max, a[i]);
    }
    *r = 1234;

    // ----------------- standalone asynchronous algorithms

    xf::cudaDeviceVector<std::byte> buf(
      xf::cuda_max_element_buffer_size<decltype(p), T>(N)
    );

    xf::cuda_max_element(p, a, a+N, r, xf::cuda_less<T>{}, buf.data());
    s.synchronize();

    if(max != std::numeric_limits<T>::lowest()) {
      REQUIRE(a[*r] == max);
    }
    else {
      REQUIRE(*r == N);
    }
    
    // ----------------- cudaflow
    *r = 1234;
    
    taskflow.emplace([&](){
      F cudaflow;
      cudaflow.max_element(a, a+N, r, xf::cuda_less<T>{});
      run_and_wait(cudaflow);
    });

    executor.run(taskflow).wait();
    
    if(max != std::numeric_limits<T>::lowest()) {
      REQUIRE(a[*r] == max);
    }
    else {
      REQUIRE(*r == N);
    }
    
    REQUIRE(cudaFree(a) == cudaSuccess);
    REQUIRE(cudaFree(r) == cudaSuccess);
  }
}

TEST_CASE("cudaflow.max_element.int" * doctest::timeout(300)) {
  max_element<int, xf::cudaFlow>();
}

TEST_CASE("cudaflow.max_element.float" * doctest::timeout(300)) {
  max_element<float, xf::cudaFlow>();
}

TEST_CASE("capturer.max_element.int" * doctest::timeout(300)) {
  max_element<int, xf::cudaFlowCapturer>();
}

TEST_CASE("capturer.max_element.float" * doctest::timeout(300)) {
  max_element<float, xf::cudaFlowCapturer>();
}

/*// --------------------------------------------------------------------------
// row-major transpose
// ----------------------------------------------------------------------------

// Disable for now - better to use cublasFlowCapturer

template <typename T>
__global__
void verify(const T* din_mat, const T* dout_mat, bool* check, size_t rows, size_t cols) {
  
  size_t tid = blockDim.x * blockIdx.x + threadIdx.x;
  size_t size = rows * cols;
  for(; tid < size; tid += gridDim.x * blockDim.x) {
    if(din_mat[tid] != dout_mat[tid / cols + (tid % cols) * rows]) {
      *check = false;
      return;
    }
  }
}

template <typename T>
void transpose() {
  xf::Executor executor;

  for(size_t rows = 1; rows <= 7999; rows*=2+3) {
    for(size_t cols = 1; cols <= 8021; cols*=3+5) {

      xf::Taskflow taskflow;
      std::vector<T> hinput_mat(rows * cols);

      std::generate_n(hinput_mat.begin(), rows * cols, [](){ return ::rand(); });

      T* dinput_mat {nullptr};
      T* doutput_mat {nullptr};
      bool* check {nullptr};
      
       //allocate
      auto allocate = taskflow.emplace([&]() {
        REQUIRE(cudaMalloc(&dinput_mat, (rows * cols) * sizeof(T)) == cudaSuccess);
        REQUIRE(cudaMalloc(&doutput_mat, (rows * cols) * sizeof(T)) == cudaSuccess);
        REQUIRE(cudaMallocManaged(&check, sizeof(bool)) == cudaSuccess);
        *check = true;
      }).name("allocate");

       //transpose
      auto cudaflow = taskflow.emplace([&](xf::cudaFlow& cf) {
        auto h2d_input_t = cf.copy(dinput_mat, hinput_mat.data(), rows * cols).name("h2d");

        auto kernel_t = xf::cudaBLAF(cf).transpose(
          dinput_mat,
          doutput_mat,
          rows,
          cols
        );

        auto verify_t = cf.kernel(
          32,
          512,
          0,
          verify<T>,
          dinput_mat,
          doutput_mat,
          check,
          rows,
          cols
        );

        h2d_input_t.precede(kernel_t);
        kernel_t.precede(verify_t);
      }).name("transpose");


       //free memory
      auto deallocate = taskflow.emplace([&](){
        REQUIRE(cudaFree(dinput_mat) == cudaSuccess);
        REQUIRE(cudaFree(doutput_mat) == cudaSuccess);
      }).name("deallocate");
      

      allocate.precede(cudaflow);
      cudaflow.precede(deallocate);

      executor.run(taskflow).wait();
      REQUIRE(*check);
    }
  }
}

TEST_CASE("transpose.int" * doctest::timeout(300) ) {
  transpose<int>();
}

TEST_CASE("transpose.float" * doctest::timeout(300) ) {
  transpose<float>();
}


TEST_CASE("transpose.double" * doctest::timeout(300) ) {
  transpose<double>();
}

// ----------------------------------------------------------------------------
// row-major matrix multiplication
// ----------------------------------------------------------------------------

template <typename T>
void matmul() {
  xf::Taskflow taskflow;
  xf::Executor executor;
  
  std::vector<T> a, b, c;

  for(int m=1; m<=1992; m=2*m+1) {
    for(int k=1; k<=1012; k=2*k+3) {
      for(int n=1; n<=1998; n=2*n+8) {

        taskflow.clear();

        T* ha {nullptr};
        T* hb {nullptr};
        T* hc {nullptr};
        T* da {nullptr};
        T* db {nullptr};
        T* dc {nullptr};
      
        T val_a = ::rand()%5-1;
        T val_b = ::rand()%7-3;

        auto hosta = taskflow.emplace([&](){ 
          a.resize(m*k);
          std::fill_n(a.begin(), m*k, val_a);
          ha = a.data();
          REQUIRE(cudaMalloc(&da, m*k*sizeof(T)) == cudaSuccess);
        }).name("ha");

        auto hostb = taskflow.emplace([&](){ 
          b.resize(k*n);
          std::fill_n(b.begin(), k*n, val_b);
          hb = b.data();
          REQUIRE(cudaMalloc(&db, k*n*sizeof(T)) == cudaSuccess);
        }).name("hb");

        auto hostc = taskflow.emplace([&](){
          c.resize(m*n);
          hc = c.data();
          REQUIRE(cudaMalloc(&dc, m*n*sizeof(T)) == cudaSuccess);
        }).name("hc");

        auto cuda = taskflow.emplace([&](xf::cudaFlow& cf){
          auto pa = cf.copy(da, ha, m*k);
          auto pb = cf.copy(db, hb, k*n);

          auto op = xf::cudaBLAF(cf).matmul(
            da, db, dc, m, k, n 
          ).name("op");

          auto cc = cf.copy(hc, dc, m*n).name("cc");

          op.precede(cc).succeed(pa, pb);
        });

        cuda.succeed(hosta, hostb, hostc);

        executor.run(taskflow).wait();

        int ans = val_a*val_b*k;
        for(const auto& x : c) {
          REQUIRE((int)x == ans);
        }

        REQUIRE(cudaFree(da) == cudaSuccess);
        REQUIRE(cudaFree(db) == cudaSuccess);
        REQUIRE(cudaFree(dc) == cudaSuccess);
      }
    }
  }
}

TEST_CASE("matmul.int" * doctest::timeout(300) ) {
  matmul<int>();
}

TEST_CASE("matmul.float" * doctest::timeout(300) ) {
  matmul<float>();
}

TEST_CASE("matmul.double" * doctest::timeout(300) ) {
  matmul<double>();
}*/

