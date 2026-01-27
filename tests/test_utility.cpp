#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest.h>

#include <xflow/utility/os.hpp>
//#include <xflow/utility/object_pool.hpp>
#include <xflow/utility/small_vector.hpp>
#include <xflow/utility/uuid.hpp>
#include <xflow/utility/iterator.hpp>
#include <xflow/utility/math.hpp>
#include <xflow/utility/traits.hpp>

// --------------------------------------------------------
// Testcase: SmallVector
// --------------------------------------------------------
TEST_CASE("SmallVector" * doctest::timeout(300)) {

  //SUBCASE("constructor")
  {
    xf::SmallVector<int> vec1;
    REQUIRE(vec1.size() == 0);
    REQUIRE(vec1.empty() == true);

    xf::SmallVector<int, 4> vec2;
    REQUIRE(vec2.data() != nullptr);
    REQUIRE(vec2.size() == 0);
    REQUIRE(vec2.empty() == true);
    REQUIRE(vec2.capacity() == 4);
  }

  //SUBCASE("constructor_n")
  {
    for(int N=0; N<=65536; N = (N ? N << 1 : 1)) {
      xf::SmallVector<int> vec(N);
      REQUIRE(vec.size() == N);
      REQUIRE(vec.empty() == (N == 0));
      REQUIRE(vec.max_size() >= vec.size());
      REQUIRE(vec.capacity() >= vec.size());
    }
  }

  //SUBCASE("copy_constructor")
  {
    for(int N=0; N<=65536; N = (N ? N << 1 : 1)) {
      xf::SmallVector<int> vec1(N);
      for(auto& item : vec1) {
        item = N;
      }

      xf::SmallVector<int> vec2(vec1);
      REQUIRE(vec1.size() == N);
      REQUIRE(vec2.size() == N);
      for(size_t i=0; i<vec1.size(); ++i) {
        REQUIRE(vec1[i] == vec2[i]);
        REQUIRE(vec1[i] == N);
      }
    }
  }

  //SUBCASE("move_constructor")
  {
    for(int N=0; N<=65536; N = (N ? N << 1 : 1)) {
      xf::SmallVector<int> vec1(N);
      for(auto& item : vec1) {
        item = N;
      }

      xf::SmallVector<int> vec2(std::move(vec1));
      REQUIRE(vec1.size() == 0);
      REQUIRE(vec1.empty() == true);
      REQUIRE(vec2.size() == N);

      for(size_t i=0; i<vec2.size(); ++i) {
        REQUIRE(vec2[i] == N);
      }
    }
  }

  //SUBCASE("push_back")
  {
    for(int N=0; N<=65536; N = (N ? N << 1 : 1)) {
      xf::SmallVector<int> vec;
      size_t pcap {0};
      size_t ncap {0};
      for(int n=0; n<N; ++n) {
        vec.push_back(n);
        REQUIRE(vec.size() == n+1);
        ncap = vec.capacity();
        REQUIRE(ncap >= pcap);
        pcap = ncap;
      }
      for(int n=0; n<N; ++n) {
        REQUIRE(vec[n] == n);
      }
      REQUIRE(vec.empty() == (N == 0));
    }
  }

  //SUBCASE("pop_back")
  {
    size_t size {0};
    size_t pcap {0};
    size_t ncap {0};
    xf::SmallVector<int> vec;
    for(int N=0; N<=65536; N = (N ? N << 1 : N + 1)) {
      vec.push_back(N);
      ++size;
      REQUIRE(vec.size() == size);
      if(N % 4 == 0) {
        vec.pop_back();
        --size;
        REQUIRE(vec.size() == size);
      }
      ncap = vec.capacity();
      REQUIRE(ncap >= pcap);
      pcap = ncap;
    }
    REQUIRE(vec.size() == size);
    for(size_t i=0; i<vec.size(); ++i) {
      REQUIRE(vec[i] % 4 != 0);
    }
  }

  //SUBCASE("iterator")
  {
    for(int N=0; N<=65536; N = (N ? N << 1 : 1)) {
      xf::SmallVector<int> vec;
      for(int n=0; n<N; ++n) {
        vec.push_back(n);
        REQUIRE(vec.size() == n+1);
      }

      // non-constant iterator
      {
        int val {0};
        for(auto item : vec) {
          REQUIRE(item == val);
          ++val;
        }
      }

      // constant iterator
      {
        int val {0};
        for(const auto& item : vec) {
          REQUIRE(item == val);
          ++val;
        }
      }

      // change the value
      {
        for(auto& item : vec) {
          item = 1234;
        }
        for(auto& item : vec) {
          REQUIRE(item == 1234);
        }
      }
    }
  }

  //SUBCASE("clear")
  {
    for(int N=0; N<=65536; N = (N ? N << 1 : 1)) {
      xf::SmallVector<int> vec(N);
      auto cap = vec.capacity();
      REQUIRE(vec.size() == N);
      vec.clear();
      REQUIRE(vec.size() == 0);
      REQUIRE(vec.capacity() == cap);
    }
  }

  //SUBCASE("comparison")
  {
    for(int N=0; N<=65536; N = (N ? N << 1 : 1)) {
      xf::SmallVector<int> vec1;
      for(int i=0; i<N; ++i) {
        vec1.push_back(i);
      }
      xf::SmallVector<int> vec2(vec1);
      REQUIRE(vec1 == vec2);
    }
  }
}

// --------------------------------------------------------
// Testcase: distance
// --------------------------------------------------------
TEST_CASE("distance.integral" * doctest::timeout(300)) {

  auto count = [] (int beg, int end, int step) {
    size_t c = 0;
    for(int i=beg; step > 0 ? i < end : i > end; i += step) {
      ++c;
    }
    return c;
  };

  for(int beg=-50; beg<=50; ++beg) {
    for(int end=-50; end<=50; ++end) {
      if(beg < end) {   // positive step
        for(int s=1; s<=50; s++) {
          REQUIRE((xf::distance(beg, end, s) == count(beg, end, s)));
        }
      }
      else {            // negative step
        for(int s=-1; s>=-50; s--) {
          REQUIRE((xf::distance(beg, end, s) == count(beg, end, s)));
        }
      }
    }
  }

}

// --------------------------------------------------------
// Testcase: ObjectPool.Sequential
// --------------------------------------------------------
/*
// Due to random # generation, this threaded program has a bug
void test_threaded_uuid(size_t N) {

  std::vector<xf::UUID> uuids(65536);

  // threaded
  std::mutex mutex;
  std::vector<std::thread> threads;

  for(size_t i=0; i<N; ++i) {
    threads.emplace_back([&](){
      for(int j=0; j<1000; ++j) {
        std::lock_guard<std::mutex> lock(mutex);
        uuids.push_back(xf::UUID());
      }
    });
  }

  for(auto& t : threads) {
    t.join();
  }

  auto size = uuids.size();
  std::sort(uuids.begin(), uuids.end());
  auto it = std::unique(uuids.begin(), uuids.end());
  REQUIRE(it - uuids.begin() == size);
}

TEST_CASE("uuid.10threads") {
  test_threaded_uuid(10);
}

TEST_CASE("uuid.100threads") {
  test_threaded_uuid(100);
}
*/

TEST_CASE("uuid") {

  xf::UUID u1, u2, u3, u4;

  // Comparator.
  REQUIRE(u1 == u1);

  // Copy
  u2 = u1;
  REQUIRE(u1 == u2);

  // Move
  u3 = std::move(u1);
  REQUIRE(u2 == u3);

  // Copy constructor
  xf::UUID u5(u4);
  REQUIRE(u5 == u4);

  // Move constructor.
  xf::UUID u6(std::move(u4));
  REQUIRE(u5 == u6);

  // Uniqueness
  std::vector<xf::UUID> uuids(65536);
  std::sort(uuids.begin(), uuids.end());
  auto it = std::unique(uuids.begin(), uuids.end());
  REQUIRE(it - uuids.begin() == 65536);

}



/*

// --------------------------------------------------------
// Testcase: ObjectPool.Sequential
// --------------------------------------------------------
struct Poolable {
  std::string str;
  std::vector<int> vec;
  int a;
  char b;

  TF_ENABLE_POOLABLE_ON_THIS;
};

TEST_CASE("ObjectPool.Sequential" * doctest::timeout(300)) {

  for(unsigned w=1; w<=4; w++) {

    xf::ObjectPool<Poolable> pool(w);

    REQUIRE(pool.num_heaps() > 0);
    REQUIRE(pool.num_local_heaps() > 0);
    REQUIRE(pool.num_global_heaps() > 0);
    REQUIRE(pool.num_bins_per_local_heap() > 0);
    REQUIRE(pool.num_objects_per_bin() > 0);
    REQUIRE(pool.num_objects_per_block() > 0);
    REQUIRE(pool.emptiness_threshold() > 0);

    // fill out all objects
    size_t N = 100*pool.num_objects_per_block();

    std::set<Poolable*> set;

    for(size_t i=0; i<N; ++i) {
      auto item = pool.animate();
      REQUIRE(set.find(item) == set.end());
      set.insert(item);
    }

    REQUIRE(set.size() == N);

    for(auto s : set) {
      pool.recycle(s);
    }

    REQUIRE(N == pool.capacity());
    REQUIRE(N == pool.num_available_objects());
    REQUIRE(0 == pool.num_allocated_objects());

    for(size_t i=0; i<N; ++i) {
      auto item = pool.animate();
      REQUIRE(set.find(item) != set.end());
    }

    REQUIRE(pool.num_available_objects() == 0);
    REQUIRE(pool.num_allocated_objects() == N);
  }
}

// --------------------------------------------------------
// Testcase: ObjectPool.Threaded
// --------------------------------------------------------

template <typename T>
void threaded_objectpool(unsigned W) {

  xf::ObjectPool<T> pool;

  std::vector<std::thread> threads;

  for(unsigned w=0; w<W; ++w) {
    threads.emplace_back([&pool](){
      std::vector<T*> items;
      for(int i=0; i<65536; ++i) {
        auto item = pool.animate();
        items.push_back(item);
      }
      for(auto item : items) {
        pool.recycle(item);
      }
    });
  }

  for(auto& thread : threads) {
    thread.join();
  }

  REQUIRE(pool.num_allocated_objects() == 0);
  REQUIRE(pool.num_available_objects() == pool.capacity());
}

TEST_CASE("ObjectPool.1thread" * doctest::timeout(300)) {
  threaded_objectpool<Poolable>(1);
}

TEST_CASE("ObjectPool.2threads" * doctest::timeout(300)) {
  threaded_objectpool<Poolable>(2);
}

TEST_CASE("ObjectPool.3threads" * doctest::timeout(300)) {
  threaded_objectpool<Poolable>(3);
}

TEST_CASE("ObjectPool.4threads" * doctest::timeout(300)) {
  threaded_objectpool<Poolable>(4);
}

TEST_CASE("ObjectPool.5threads" * doctest::timeout(300)) {
  threaded_objectpool<Poolable>(5);
}

TEST_CASE("ObjectPool.6threads" * doctest::timeout(300)) {
  threaded_objectpool<Poolable>(6);
}

TEST_CASE("ObjectPool.7threads" * doctest::timeout(300)) {
  threaded_objectpool<Poolable>(7);
}

TEST_CASE("ObjectPool.8threads" * doctest::timeout(300)) {
  threaded_objectpool<Poolable>(8);
}

TEST_CASE("ObjectPool.9threads" * doctest::timeout(300)) {
  threaded_objectpool<Poolable>(9);
}

TEST_CASE("ObjectPool.10threads" * doctest::timeout(300)) {
  threaded_objectpool<Poolable>(10);
}

TEST_CASE("ObjectPool.11threads" * doctest::timeout(300)) {
  threaded_objectpool<Poolable>(11);
}

TEST_CASE("ObjectPool.12threads" * doctest::timeout(300)) {
  threaded_objectpool<Poolable>(12);
}

TEST_CASE("ObjectPool.13threads" * doctest::timeout(300)) {
  threaded_objectpool<Poolable>(13);
}

TEST_CASE("ObjectPool.14threads" * doctest::timeout(300)) {
  threaded_objectpool<Poolable>(14);
}

TEST_CASE("ObjectPool.15threads" * doctest::timeout(300)) {
  threaded_objectpool<Poolable>(15);
}

TEST_CASE("ObjectPool.16threads" * doctest::timeout(300)) {
  threaded_objectpool<Poolable>(16);
}
*/


// --------------------------------------------------------
// Testcase: Reference Wrapper
// --------------------------------------------------------

TEST_CASE("RefWrapper" * doctest::timeout(300)) {

  static_assert(std::is_same<
    xf::unwrap_reference_t<int>, int
  >::value, "");

  static_assert(std::is_same<
    xf::unwrap_reference_t<int&>, int&
  >::value, "");

  static_assert(std::is_same<
    xf::unwrap_reference_t<int&&>, int&&
  >::value, "");

  static_assert(std::is_same<
    xf::unwrap_reference_t<std::reference_wrapper<int>>, int&
  >::value, "");

  static_assert(std::is_same<
    xf::unwrap_reference_t<std::reference_wrapper<std::reference_wrapper<int>>>,
    std::reference_wrapper<int>&
  >::value, "");

  static_assert(std::is_same<
    xf::unwrap_ref_decay_t<int>, int
  >::value, "");

  static_assert(std::is_same<
    xf::unwrap_ref_decay_t<int&>, int
  >::value, "");

  static_assert(std::is_same<
    xf::unwrap_ref_decay_t<int&&>, int
  >::value, "");

  static_assert(std::is_same<
    xf::unwrap_ref_decay_t<std::reference_wrapper<int>>, int&
  >::value, "");

  static_assert(std::is_same<
    xf::unwrap_ref_decay_t<std::reference_wrapper<std::reference_wrapper<int>>>,
    std::reference_wrapper<int>&
  >::value, "");

}

//// --------------------------------------------------------
//// Testcase: FunctionTraits
//// --------------------------------------------------------
//void func1() {
//}
//
//int func2(int, double, float, char) {
//  return 0;
//}
//
//TEST_CASE("FunctionTraits" * doctest::timeout(300)) {
//
//  SUBCASE("func1") {
//    using func1_traits = xf::function_traits<decltype(func1)>;
//    static_assert(std::is_same<func1_traits::return_type, void>::value, "");
//    static_assert(func1_traits::arity == 0, "");
//  }
//
//  SUBCASE("func2") {
//    using func2_traits = xf::function_traits<decltype(func2)>;
//    static_assert(std::is_same<func2_traits::return_type, int>::value, "");
//    static_assert(func2_traits::arity == 4, "");
//    static_assert(std::is_same<func2_traits::argument_t<0>, int>::value,   "");
//    static_assert(std::is_same<func2_traits::argument_t<1>, double>::value,"");
//    static_assert(std::is_same<func2_traits::argument_t<2>, float>::value, "");
//    static_assert(std::is_same<func2_traits::argument_t<3>, char>::value,  "");
//  }
//
//  SUBCASE("lambda1") {
//    auto lambda1 = [] () mutable {
//      return 1;
//    };
//    using lambda1_traits = xf::function_traits<decltype(lambda1)>;
//    static_assert(std::is_same<lambda1_traits::return_type, int>::value, "");
//    static_assert(lambda1_traits::arity == 0, "");
//  }
//
//  SUBCASE("lambda2") {
//    auto lambda2 = [] (int, double, char&) {
//    };
//    using lambda2_traits = xf::function_traits<decltype(lambda2)>;
//    static_assert(std::is_same<lambda2_traits::return_type, void>::value, "");
//    static_assert(lambda2_traits::arity == 3, "");
//    static_assert(std::is_same<lambda2_traits::argument_t<0>, int>::value, "");
//    static_assert(std::is_same<lambda2_traits::argument_t<1>, double>::value, "");
//    static_assert(std::is_same<lambda2_traits::argument_t<2>, char&>::value, "");
//  }
//
//  SUBCASE("class") {
//    struct foo {
//      int operator ()(int, float) const;
//    };
//    using foo_traits = xf::function_traits<foo>;
//    static_assert(std::is_same<foo_traits::return_type, int>::value, "");
//    static_assert(foo_traits::arity == 2, "");
//    static_assert(std::is_same<foo_traits::argument_t<0>, int>::value, "");
//    static_assert(std::is_same<foo_traits::argument_t<1>, float>::value, "");
//  }
//
//  SUBCASE("std-function") {
//    using ft1 = xf::function_traits<std::function<void()>>;
//    static_assert(std::is_same<ft1::return_type, void>::value, "");
//    static_assert(ft1::arity == 0, "");
//
//    using ft2 = xf::function_traits<std::function<int(int&, double&&)>&>;
//    static_assert(std::is_same<ft2::return_type, int>::value, "");
//    static_assert(ft2::arity == 2, "");
//    static_assert(std::is_same<ft2::argument_t<0>, int&>::value, "");
//    static_assert(std::is_same<ft2::argument_t<1>, double&&>::value, "");
//
//    using ft3 = xf::function_traits<std::function<int(int&, double&&)>&&>;
//    static_assert(std::is_same<ft3::return_type, int>::value, "");
//    static_assert(ft3::arity == 2, "");
//    static_assert(std::is_same<ft3::argument_t<0>, int&>::value, "");
//    static_assert(std::is_same<ft3::argument_t<1>, double&&>::value, "");
//
//    using ft4 = xf::function_traits<const std::function<void(int)>&>;
//    static_assert(std::is_same<ft4::return_type, void>::value, "");
//    static_assert(ft4::arity == 1, "");
//    static_assert(std::is_same<ft4::argument_t<0>, int>::value, "");
//  }
//}

// --------------------------------------------------------
// Math utilities
// --------------------------------------------------------
TEST_CASE("NextPow2") {

  static_assert(xf::next_pow2(0u) == 1);
  static_assert(xf::next_pow2(1u) == 1);
  static_assert(xf::next_pow2(100u) == 128u);
  static_assert(xf::next_pow2(245u) == 256u);
  static_assert(xf::next_pow2(512u) == 512u);
  static_assert(xf::next_pow2(513u) == 1024u);

  REQUIRE(xf::next_pow2(0u) == 1u);
  REQUIRE(xf::next_pow2(2u) == 2u);
  REQUIRE(xf::next_pow2(1u) == 1u);
  REQUIRE(xf::next_pow2(33u) == 64u);
  REQUIRE(xf::next_pow2(100u) == 128u);
  REQUIRE(xf::next_pow2(211u) == 256u);
  REQUIRE(xf::next_pow2(23u) == 32u);
  REQUIRE(xf::next_pow2(54u) == 64u);

  uint64_t z = 0;
  uint64_t a = 1;
  REQUIRE(xf::next_pow2(z) == 1);
  REQUIRE(xf::next_pow2(a) == a);
  REQUIRE(xf::next_pow2((a<<5)  + 0) == (a<<5));
  REQUIRE(xf::next_pow2((a<<5)  + 1) == (a<<6));
  REQUIRE(xf::next_pow2((a<<32) + 0) == (a<<32));
  REQUIRE(xf::next_pow2((a<<32) + 1) == (a<<33));
  REQUIRE(xf::next_pow2((a<<41) + 0) == (a<<41));
  REQUIRE(xf::next_pow2((a<<41) + 1) == (a<<42));

  REQUIRE(xf::is_pow2(0) == false);
  REQUIRE(xf::is_pow2(1) == true);
  REQUIRE(xf::is_pow2(2) == true);
  REQUIRE(xf::is_pow2(3) == false);
  REQUIRE(xf::is_pow2(0u) == false);
  REQUIRE(xf::is_pow2(1u) == true);
  REQUIRE(xf::is_pow2(54u) == false);
  REQUIRE(xf::is_pow2(64u) == true);
}

// ----------------------------------------------------------------------------
// count the number of trailing zeros
// ----------------------------------------------------------------------------

TEST_CASE("CTZ") {
  REQUIRE(xf::ctz<uint32_t>(0b00000001) == 0);
  REQUIRE(xf::ctz<uint32_t>(0b00000010) == 1);
  REQUIRE(xf::ctz<uint32_t>(0b00000100) == 2);
  REQUIRE(xf::ctz<uint32_t>(0b10000000) == 7);

  REQUIRE(xf::ctz<uint64_t>(0b00000001ULL) == 0);
  REQUIRE(xf::ctz<uint64_t>(0b00000010ULL) == 1);
  REQUIRE(xf::ctz<uint64_t>(0b00000100ULL) == 2);
  REQUIRE(xf::ctz<uint64_t>(0x8000000000000000ULL) == 63);

  //REQUIRE(xf::ctz<uint32_t>(0) == 32); // Undefined behavior, doesn't work for Windows
  REQUIRE(xf::ctz<uint32_t>(0xFFFFFFFF) == 0);
  REQUIRE(xf::ctz<uint32_t>(0x00010000) == 16);
  REQUIRE(xf::ctz<uint32_t>(0x80000000) == 31);

  //REQUIRE(xf::ctz<uint64_t>(0) == 64); // Undefined behavior, doesn't work for Windows
  REQUIRE(xf::ctz<uint64_t>(0xFFFFFFFFFFFFFFFFULL) == 0);
  REQUIRE(xf::ctz<uint64_t>(0x0000000100000000ULL) == 32);
  REQUIRE(xf::ctz<uint64_t>(0x0000000000008000ULL) == 15);
  REQUIRE(xf::ctz<uint64_t>(0x4000000000000000ULL) == 62);
}

// ----------------------------------------------------------------------------
// test coprimes
// ----------------------------------------------------------------------------

TEST_CASE("Coprimes") {

  // Compile-time checks for known values
  static_assert(xf::coprime(1) == 1);
  static_assert(xf::coprime(2) == 1);
  static_assert(xf::coprime(3) == 2);
  static_assert(xf::coprime(4) == 3);
  static_assert(xf::coprime(5) == 4);
  static_assert(xf::coprime(6) == 5);
  static_assert(xf::coprime(7) == 6);
  static_assert(xf::coprime(8) == 7);
  static_assert(xf::coprime(9) == 8);
  static_assert(xf::coprime(10) == 9);
  static_assert(xf::coprime(11) == 10);
  static_assert(xf::coprime(12) == 11);
  static_assert(xf::coprime(13) == 12);
  static_assert(xf::coprime(14) == 13);
  static_assert(xf::coprime(15) == 14);
  static_assert(xf::coprime(16) == 15);
  static_assert(xf::coprime(17) == 16);
  static_assert(xf::coprime(18) == 17);
  static_assert(xf::coprime(19) == 18);
  static_assert(xf::coprime(20) == 19);

  constexpr auto coprime_table = xf::make_coprime_lut<51>();
  
  static_assert(coprime_table[1] == 1);
  static_assert(coprime_table[2] == 1);
  static_assert(coprime_table[3] == 2);
  static_assert(coprime_table[4] == 3);
  static_assert(coprime_table[5] == 4);
  static_assert(coprime_table[6] == 5);
  static_assert(coprime_table[7] == 6);
  static_assert(coprime_table[8] == 7);
  static_assert(coprime_table[9] == 8);
  static_assert(coprime_table[10] == 9);
  static_assert(coprime_table[11] == 10);
  static_assert(coprime_table[12] == 11);
  static_assert(coprime_table[13] == 12);
  static_assert(coprime_table[14] == 13);
  static_assert(coprime_table[15] == 14);
  static_assert(coprime_table[16] == 15);
  static_assert(coprime_table[17] == 16);
  static_assert(coprime_table[18] == 17);
  static_assert(coprime_table[19] == 18);
  static_assert(coprime_table[20] == 19);

  // Runtime assertions for all values up to 50
  for (size_t i = 1; i <= 50; ++i) {
    REQUIRE(std::gcd(i, coprime_table[i]) == 1);
    REQUIRE(xf::coprime(i) == coprime_table[i]);
    
    // randomly generate a coprime
    auto v = ::rand() % 1048 + 2;
    auto c = xf::coprime(v);
    REQUIRE(std::gcd(v, c) == 1);
    REQUIRE(c < v);
  }
  
}

// ----------------------------------------------------------------------------
// Log2
// ----------------------------------------------------------------------------

TEST_CASE("FloorLog2") {

  REQUIRE(xf::floor_log2(1u) == 0);
  REQUIRE(xf::floor_log2(2u) == 1);
  REQUIRE(xf::floor_log2(4u) == 2);
  REQUIRE(xf::floor_log2(8u) == 3);
  REQUIRE(xf::floor_log2(16u) == 4);
  REQUIRE(xf::floor_log2(32u) == 5);
  REQUIRE(xf::floor_log2(64u) == 6);
  REQUIRE(xf::floor_log2(128u) == 7);
  REQUIRE(xf::floor_log2(256u) == 8);

  // Test non-powers of 2 (floor log2)
  REQUIRE(xf::floor_log2(3u) == 1);
  REQUIRE(xf::floor_log2(5u) == 2);
  REQUIRE(xf::floor_log2(6u) == 2);
  REQUIRE(xf::floor_log2(7u) == 2);
  REQUIRE(xf::floor_log2(9u) == 3);
  REQUIRE(xf::floor_log2(10u) == 3);
  REQUIRE(xf::floor_log2(15u) == 3);
  REQUIRE(xf::floor_log2(17u) == 4);
  REQUIRE(xf::floor_log2(31u) == 4);
  REQUIRE(xf::floor_log2(33u) == 5);
  
  // Test large values
  REQUIRE(xf::floor_log2(1023u) == 9);
  REQUIRE(xf::floor_log2(1024u) == 10);
  REQUIRE(xf::floor_log2(1025u) == 10);
  REQUIRE(xf::floor_log2(std::numeric_limits<uint32_t>::max()) == 31);
  REQUIRE(xf::floor_log2(std::numeric_limits<uint64_t>::max()) == 63);
}



