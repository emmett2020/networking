#include <catch2/catch_test_macros.hpp>
#include "utils/then_value.h"
#include <stdexec/execution.hpp>

TEST_CASE("Use lambda as function, the lambda uses empty capture list.", "[utils.then_value]") {
  stdexec::sender auto task = then_value(
    stdexec::just(1), [](int a, int b) { return a + b; }, 2);
  auto ret = stdexec::sync_wait(std::move(task));
  CHECK(ret.has_value());
  CHECK(std::get<0>(ret.value()) == 3);
}

TEST_CASE("test lambda with reference capture list", "[utils.then_value]") {
  int b = 2;
  stdexec::sender auto task = then_value(stdexec::just(1), [&b](int a) { return a + b; });
  auto ret = stdexec::sync_wait(std::move(task));
  CHECK(ret.has_value());
  CHECK(std::get<0>(ret.value()) == 3);
}

TEST_CASE("reference capture list", "[utils.then_value]") {
  int b = 2;
  stdexec::sender auto task = then_value(stdexec::just(1), [&b](int a) { b = 3; });
  auto ret = stdexec::sync_wait(std::move(task));
  CHECK(ret.has_value());
  CHECK(b == 3);
}

struct NonCopyable {
  NonCopyable() = default;
  NonCopyable(const NonCopyable&) = delete;
  NonCopyable& operator=(const NonCopyable&) = delete;
  int value = 0;
};

struct Foo {
  void MemberFunc(int n, int& m) {
    m = n;
  }
};

TEST_CASE("Use non-copyable arguments ", "[utils.then_value]") {
  NonCopyable non_copyable;

  stdexec::sender auto task = then_value(stdexec::just(1), [&non_copyable](int a) {
    non_copyable.value = 3;
  });
  auto ret = stdexec::sync_wait(std::move(task));
  CHECK(ret.has_value());
  CHECK(non_copyable.value == 3);
}

TEST_CASE("Test member func arguments ", "[utils.then_value]") {
  Foo foo;
  int m = 0;
  stdexec::sender auto task = then_value(stdexec::just(1), foo, &Foo::MemberFunc, m);
  auto ret = stdexec::sync_wait(std::move(task));
  CHECK(ret.has_value());
  CHECK(m == 1);
}

TEST_CASE("test lambda with void return", "[utils.then_value]") {
  stdexec::sender auto task = then_value(stdexec::just(1), [](int a) {});
  auto ret = stdexec::sync_wait(std::move(task));
  CHECK(ret.has_value());
}

TEST_CASE("test the order of then_value_t", "[utils.then_value]") {
  int num = 0;
  stdexec::sender auto prev = stdexec::then(stdexec::just(), [&num] {
    CHECK(num == 0);
    ++num;
  });
  stdexec::sender auto task = then_value(std::move(prev), [&num] {
    CHECK(num == 1);
    return ++num;
  });
  auto ret = stdexec::sync_wait(std::move(task));
  CHECK(ret.has_value());
  CHECK(std::get<0>(ret.value()) == 2);
}

TEST_CASE("then_value with then_value", "[utils.then_value]") {
  int num = 0;
  stdexec::sender auto start = then_value(stdexec::just(), [&num] {
    CHECK(num == 0);
    ++num;
  });
  stdexec::sender auto task1 = then_value(std::move(start), [&num] {
    CHECK(num == 1);
    ++num;
  });
  stdexec::sender auto task2 = then_value(std::move(task1), [&num] {
    CHECK(num == 2);
    ++num;
  });
  stdexec::sender auto task3 = then_value(std::move(task2), [&num] {
    CHECK(num == 3);
    ++num;
  });
  stdexec::sender auto task4 = then_value(std::move(task3), [&num] {
    CHECK(num == 4);
    ++num;
  });
  stdexec::sender auto task5 = then_value(std::move(task4), [&num] {
    CHECK(num == 5);
    return ++num;
  });

  auto ret = stdexec::sync_wait(std::move(task5));
  CHECK(ret.has_value());
  CHECK(std::get<0>(ret.value()) == 6);
}
