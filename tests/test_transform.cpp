#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest.h>
#include <xflow/taskflow.hpp>
#include <xflow/algorithm/transform.hpp>

// ----------------------------------------------------------------------------
// Parallel Transform 1
// ----------------------------------------------------------------------------

template<typename T, typename P>
void parallel_transform(size_t W) {

  std::srand(static_cast<unsigned int>(time(NULL)));

  xf::Taskflow taskflow;
  xf::Executor executor(W);

  for(size_t N=0; N<1000; N=(N+1)*2) {
    for(size_t c : {0, 1, 3, 7, 99}) {

      taskflow.clear();

      typename T::const_iterator src_beg;
      typename T::const_iterator src_end;
      std::list<std::string>::iterator tgt_beg;

      T src;
      std::list<std::string> tgt;

      taskflow.clear();

      auto from = taskflow.emplace([&](){
        src.resize(N);
        for(auto& d : src) {
          d = ::rand() % 10;
          tgt.emplace_back("hi");
        }
        src_beg = src.begin();
        src_end = src.end();
        tgt_beg = tgt.begin();
      });

      auto to = taskflow.transform(
        std::ref(src_beg), std::ref(src_end), std::ref(tgt_beg),
        [] (const auto& in) {
          return std::to_string(in+10);
        },
        P(c)
      );

      from.precede(to);

      executor.run(taskflow).wait();

      auto s_itr = src.begin();
      auto d_itr = tgt.begin();
      while(s_itr != src.end()) {
        REQUIRE(*d_itr++ == std::to_string(*s_itr++ + 10));
      }
    }
  }
}

// guided
TEST_CASE("ParallelTransform.Guided.1thread") {
  parallel_transform<std::vector<int>, xf::GuidedPartitioner<>>(1);
  parallel_transform<std::list<int>, xf::GuidedPartitioner<>>(1);
}

TEST_CASE("ParallelTransform.Guided.2threads") {
  parallel_transform<std::vector<int>, xf::GuidedPartitioner<>>(2);
  parallel_transform<std::list<int>, xf::GuidedPartitioner<>>(2);
}

TEST_CASE("ParallelTransform.Guided.3threads") {
  parallel_transform<std::vector<int>, xf::GuidedPartitioner<>>(3);
  parallel_transform<std::list<int>, xf::GuidedPartitioner<>>(3);
}

TEST_CASE("ParallelTransform.Guided.4threads") {
  parallel_transform<std::vector<int>, xf::GuidedPartitioner<>>(4);
  parallel_transform<std::list<int>, xf::GuidedPartitioner<>>(4);
}

// random
TEST_CASE("ParallelTransform.Random.1thread") {
  parallel_transform<std::vector<int>, xf::RandomPartitioner<>>(1);
  parallel_transform<std::list<int>, xf::RandomPartitioner<>>(1);
}

TEST_CASE("ParallelTransform.Random.2threads") {
  parallel_transform<std::vector<int>, xf::RandomPartitioner<>>(2);
  parallel_transform<std::list<int>, xf::RandomPartitioner<>>(2);
}

TEST_CASE("ParallelTransform.Random.3threads") {
  parallel_transform<std::vector<int>, xf::RandomPartitioner<>>(3);
  parallel_transform<std::list<int>, xf::RandomPartitioner<>>(3);
}

TEST_CASE("ParallelTransform.Random.4threads") {
  parallel_transform<std::vector<int>, xf::RandomPartitioner<>>(4);
  parallel_transform<std::list<int>, xf::RandomPartitioner<>>(4);
}

// static
TEST_CASE("ParallelTransform.Static.1thread") {
  parallel_transform<std::vector<int>, xf::StaticPartitioner<>>(1);
  parallel_transform<std::list<int>, xf::StaticPartitioner<>>(1);
}

TEST_CASE("ParallelTransform.Static.2threads") {
  parallel_transform<std::vector<int>, xf::StaticPartitioner<>>(2);
  parallel_transform<std::list<int>, xf::StaticPartitioner<>>(2);
}

TEST_CASE("ParallelTransform.Static.3threads") {
  parallel_transform<std::vector<int>, xf::StaticPartitioner<>>(3);
  parallel_transform<std::list<int>, xf::StaticPartitioner<>>(3);
}

TEST_CASE("ParallelTransform.Static.4threads") {
  parallel_transform<std::vector<int>, xf::StaticPartitioner<>>(4);
  parallel_transform<std::list<int>, xf::StaticPartitioner<>>(4);
}

// ----------------------------------------------------------------------------
// Parallel Transform 2
// ----------------------------------------------------------------------------

template<typename T, typename P>
void parallel_transform2(size_t W) {

  std::srand(static_cast<unsigned int>(time(NULL)));

  xf::Taskflow taskflow;
  xf::Executor executor(W);

  for(size_t N=0; N<1000; N=(N+1)*2) {
    for(size_t c : {0, 1, 3, 7, 99}) {

      taskflow.clear();

      typename T::const_iterator src_beg;
      typename T::const_iterator src_end;
      std::list<std::string>::iterator tgt_beg;

      T src;
      std::list<std::string> tgt;

      taskflow.clear();

      auto from = taskflow.emplace([&](){
        src.resize(N);
        for(auto& d : src) {
          d = ::rand() % 10;
          tgt.emplace_back("hi");
        }
        src_beg = src.begin();
        src_end = src.end();
        tgt_beg = tgt.begin();
      });

      auto to = taskflow.transform(
        std::ref(src_beg), std::ref(src_end), std::ref(src_beg), std::ref(tgt_beg),
        [] (const auto& in1, const auto& in2) {
          return std::to_string(in1 + in2 + 10);
        },
        P(c)
      );

      from.precede(to);

      executor.run(taskflow).wait();

      auto s_itr = src.begin();
      auto d_itr = tgt.begin();
      while(s_itr != src.end()) {
        REQUIRE(*d_itr++ == std::to_string(2 * *s_itr++ + 10));
      }
    }
  }
}

// guided
TEST_CASE("ParallelTransform2.Guided.1thread") {
  parallel_transform2<std::vector<int>, xf::GuidedPartitioner<>>(1);
  parallel_transform2<std::list<int>, xf::GuidedPartitioner<>>(1);
}

TEST_CASE("ParallelTransform2.Guided.2threads") {
  parallel_transform2<std::vector<int>, xf::GuidedPartitioner<>>(2);
  parallel_transform2<std::list<int>, xf::GuidedPartitioner<>>(2);
}

TEST_CASE("ParallelTransform2.Guided.3threads") {
  parallel_transform2<std::vector<int>, xf::GuidedPartitioner<>>(3);
  parallel_transform2<std::list<int>, xf::GuidedPartitioner<>>(3);
}

TEST_CASE("ParallelTransform2.Guided.4threads") {
  parallel_transform2<std::vector<int>, xf::GuidedPartitioner<>>(4);
  parallel_transform2<std::list<int>, xf::GuidedPartitioner<>>(4);
}

// dynamic
TEST_CASE("ParallelTransform2.Dynamic.1thread") {
  parallel_transform2<std::vector<int>, xf::DynamicPartitioner<>>(1);
  parallel_transform2<std::list<int>, xf::DynamicPartitioner<>>(1);
}

TEST_CASE("ParallelTransform2.Dynamic.2threads") {
  parallel_transform2<std::vector<int>, xf::DynamicPartitioner<>>(2);
  parallel_transform2<std::list<int>, xf::DynamicPartitioner<>>(2);
}

TEST_CASE("ParallelTransform2.Dynamic.3threads") {
  parallel_transform2<std::vector<int>, xf::DynamicPartitioner<>>(3);
  parallel_transform2<std::list<int>, xf::DynamicPartitioner<>>(3);
}

TEST_CASE("ParallelTransform2.Dynamic.4threads") {
  parallel_transform2<std::vector<int>, xf::DynamicPartitioner<>>(4);
  parallel_transform2<std::list<int>, xf::DynamicPartitioner<>>(4);
}

// static
TEST_CASE("ParallelTransform2.Static.1thread") {
  parallel_transform2<std::vector<int>, xf::StaticPartitioner<>>(1);
  parallel_transform2<std::list<int>, xf::StaticPartitioner<>>(1);
}

TEST_CASE("ParallelTransform2.Static.2threads") {
  parallel_transform2<std::vector<int>, xf::StaticPartitioner<>>(2);
  parallel_transform2<std::list<int>, xf::StaticPartitioner<>>(2);
}

TEST_CASE("ParallelTransform2.Static.3threads") {
  parallel_transform2<std::vector<int>, xf::StaticPartitioner<>>(3);
  parallel_transform2<std::list<int>, xf::StaticPartitioner<>>(3);
}

TEST_CASE("ParallelTransform2.Static.4threads") {
  parallel_transform2<std::vector<int>, xf::StaticPartitioner<>>(4);
  parallel_transform2<std::list<int>, xf::StaticPartitioner<>>(4);
}

// random
TEST_CASE("ParallelTransform2.Random.1thread") {
  parallel_transform2<std::vector<int>, xf::RandomPartitioner<>>(1);
  parallel_transform2<std::list<int>, xf::RandomPartitioner<>>(1);
}

TEST_CASE("ParallelTransform2.Random.2threads") {
  parallel_transform2<std::vector<int>, xf::RandomPartitioner<>>(2);
  parallel_transform2<std::list<int>, xf::RandomPartitioner<>>(2);
}

TEST_CASE("ParallelTransform2.Random.3threads") {
  parallel_transform2<std::vector<int>, xf::RandomPartitioner<>>(3);
  parallel_transform2<std::list<int>, xf::RandomPartitioner<>>(3);
}

TEST_CASE("ParallelTransform2.Random.4threads") {
  parallel_transform2<std::vector<int>, xf::RandomPartitioner<>>(4);
  parallel_transform2<std::list<int>, xf::RandomPartitioner<>>(4);
}


// ----------------------------------------------------------------------------
// Parallel Transform 3
// ----------------------------------------------------------------------------

template <typename P>
void parallel_transform3(size_t W) {

  std::srand(static_cast<unsigned int>(time(NULL)));

  xf::Taskflow taskflow;
  xf::Executor executor(W);

  using std::string;
  using std::size_t;

  for(size_t N=0; N<1000; N=(N+1)*2) {

    std::multimap<int, size_t> src;

    /** Reference implementation with std::transform */
    std::vector<string> ref;

    /** Target implementation with Subflow::transform */
    std::vector<string> tgt;

    typename std::vector<string>::iterator tgt_beg;

    /** A generic function to cast integers to string */
    const auto myFunction = [](const size_t x) -> string {
      return "id_" + std::to_string(x);
    };

    taskflow.clear();

    /** Group integers 0..(N-1) into ten groups,
     * each having an unique key `d`.
     */
    auto from = taskflow.emplace([&, N](){
      for(size_t i = 0; i < N; i++) {
        const int d = ::rand() % 10;
        src.emplace(d, i);
      }

      ref.resize(N);

      tgt.resize(N);
      tgt_beg = tgt.begin();
    });

    auto to_ref = taskflow.emplace([&]() {

      // Find entries matching key = 0.
      // This can return empty results.
      const auto [src_beg, src_end] = src.equal_range(0);
      const size_t n_matching = std::distance(src_beg, src_end);
      ref.resize(n_matching);

      // Extract all values having matching key value.
      std::transform(src_beg, src_end, ref.begin(),
        [&](const auto& x) -> string {
          return myFunction(x.second);
        }
      );
    });

    /** Dynamic scheduling with Subflow::transform */
    auto to_tgt = taskflow.emplace([&](xf::Subflow& subflow) {

      // Find entries matching key = 0
      const auto [src_beg, src_end] = src.equal_range(0);
      const size_t n_matching = std::distance(src_beg, src_end);
      tgt.resize(n_matching);

      subflow.transform(
        std::ref(src_beg), std::ref(src_end), std::ref(tgt_beg),
        [&] (const auto& x) -> string {
          return myFunction(x.second);
      }, P());

      subflow.join();
    });

    from.precede(to_ref);
    from.precede(to_tgt);

    executor.run(taskflow).wait();

    /** Target entries much match. */
    REQUIRE(std::equal(tgt.begin(), tgt.end(), ref.begin()));
  }
}

// guided
TEST_CASE("ParallelTransform3.Guided.1thread") {
  parallel_transform3<xf::GuidedPartitioner<>>(1);
  parallel_transform3<xf::GuidedPartitioner<>>(1);
}

TEST_CASE("ParallelTransform3.Guided.2threads") {
  parallel_transform3<xf::GuidedPartitioner<>>(2);
  parallel_transform3<xf::GuidedPartitioner<>>(2);
}

TEST_CASE("ParallelTransform3.Guided.3threads") {
  parallel_transform3<xf::GuidedPartitioner<>>(3);
  parallel_transform3<xf::GuidedPartitioner<>>(3);
}

TEST_CASE("ParallelTransform3.Guided.4threads") {
  parallel_transform3<xf::GuidedPartitioner<>>(4);
  parallel_transform3<xf::GuidedPartitioner<>>(4);
}

// dynamic
TEST_CASE("ParallelTransform3.Dynamic.1thread") {
  parallel_transform3<xf::DynamicPartitioner<>>(1);
  parallel_transform3<xf::DynamicPartitioner<>>(1);
}

TEST_CASE("ParallelTransform3.Dynamic.2threads") {
  parallel_transform3<xf::DynamicPartitioner<>>(2);
  parallel_transform3<xf::DynamicPartitioner<>>(2);
}

TEST_CASE("ParallelTransform3.Dynamic.3threads") {
  parallel_transform3<xf::DynamicPartitioner<>>(3);
  parallel_transform3<xf::DynamicPartitioner<>>(3);
}

TEST_CASE("ParallelTransform3.Dynamic.4threads") {
  parallel_transform3<xf::DynamicPartitioner<>>(4);
  parallel_transform3<xf::DynamicPartitioner<>>(4);
}

// static
TEST_CASE("ParallelTransform3.Static.1thread") {
  parallel_transform3<xf::StaticPartitioner<>>(1);
  parallel_transform3<xf::StaticPartitioner<>>(1);
}

TEST_CASE("ParallelTransform3.Static.2threads") {
  parallel_transform3<xf::StaticPartitioner<>>(2);
  parallel_transform3<xf::StaticPartitioner<>>(2);
}

TEST_CASE("ParallelTransform3.Static.3threads") {
  parallel_transform3<xf::StaticPartitioner<>>(3);
  parallel_transform3<xf::StaticPartitioner<>>(3);
}

TEST_CASE("ParallelTransform3.Static.4threads") {
  parallel_transform3<xf::StaticPartitioner<>>(4);
  parallel_transform3<xf::StaticPartitioner<>>(4);
}

// random
TEST_CASE("ParallelTransform3.Random.1thread") {
  parallel_transform3<xf::RandomPartitioner<>>(1);
  parallel_transform3<xf::RandomPartitioner<>>(1);
}

TEST_CASE("ParallelTransform3.Random.2threads") {
  parallel_transform3<xf::RandomPartitioner<>>(2);
  parallel_transform3<xf::RandomPartitioner<>>(2);
}

TEST_CASE("ParallelTransform3.Random.3threads") {
  parallel_transform3<xf::RandomPartitioner<>>(3);
  parallel_transform3<xf::RandomPartitioner<>>(3);
}

TEST_CASE("ParallelTransform3.Random.4threads") {
  parallel_transform3<xf::RandomPartitioner<>>(4);
  parallel_transform3<xf::RandomPartitioner<>>(4);
}

// ----------------------------------------------------------------------------
// Closure Wrapper
// ----------------------------------------------------------------------------

int& GetThreadSpecificContext()
{
    thread_local int context = 0;
    return context;
}

const int UPPER = 1000;

TEST_CASE("ClosureWrapper.transform.Static" * doctest::timeout(300))
{
  // Write a test case for using the taskwrapper on xf::transform
  for (int tc = 1; tc < 16; tc++)
  {
    xf::Executor executor(tc);
    std::atomic<int> wrapper_called_count = 0;
    xf::Taskflow taskflow;
    std::vector<int> range(UPPER, 0);
    std::vector<int> result(UPPER);
    taskflow.transform(range.begin(), range.end(), begin(result), 
      [&](int) { 
        return GetThreadSpecificContext(); 
      },
      xf::StaticPartitioner(1, [&](auto&& task){
        wrapper_called_count++;
        GetThreadSpecificContext() = tc;
        task();
        GetThreadSpecificContext() = 0;
      })
    );
    executor.run(taskflow).wait();

    REQUIRE(wrapper_called_count == tc);
    REQUIRE(result == std::vector<int>(UPPER, tc));
  }
}

// Implement for dynamic case for transform
TEST_CASE("ClosureWrapper.transform.Dynamic" * doctest::timeout(300))
{
  for (int tc = 1; tc < 16; tc++)
  {
    xf::Executor executor(tc);
    std::atomic<int> wrapper_called_count = 0;
    xf::Taskflow taskflow;
    std::vector<int> range(UPPER);
    std::iota(range.begin(), range.end(), 0);
    std::vector<int> result(UPPER);
    taskflow.transform(range.begin(), range.end(), begin(result), 
      [&](int){ 
        return GetThreadSpecificContext(); 
      },
      xf::DynamicPartitioner(1, [&](auto&& task) {
        wrapper_called_count++;
        GetThreadSpecificContext() = tc;
        task();
        GetThreadSpecificContext() = 0;
      })
    );
    executor.run(taskflow).wait();

    REQUIRE(wrapper_called_count <= tc);
    REQUIRE(result == std::vector<int>(UPPER, tc));
  }
}



//// ----------------------------------------------------------------------------
//// ParallelTransform Exception
//// ----------------------------------------------------------------------------
//
//void parallel_transform_exception(unsigned W) {
//  xf::Taskflow taskflow;
//  xf::Executor executor(W);
//
//  std::vector<int> src(1000000, 0);
//  std::vector<int> tgt(1000000, 0);
//
//  taskflow.transform(src.begin(), src.end(), tgt.begin(), [](int&) {
//    throw std::runtime_error("x");
//    return 1;
//  }); 
//  REQUIRE_THROWS_WITH_AS(executor.run(taskflow).get(), "x", std::runtime_error);
//}
//
//TEST_CASE("ParallelTransform.Exception.1thread") {
//  parallel_transform_exception(1);
//}
//
//TEST_CASE("ParallelTransform.Exception.2threads") {
//  parallel_transform_exception(2);
//}
//
//TEST_CASE("ParallelTransform.Exception.3threads") {
//  parallel_transform_exception(3);
//}
//
//TEST_CASE("ParallelTransform.Exception.4threads") {
//  parallel_transform_exception(4);
//}



