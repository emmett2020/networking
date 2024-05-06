#include <type_traits>

#include <catch2/catch_test_macros.hpp>
#include <stdexec/execution.hpp>

#include "net/utils/timeout.h"

using namespace exec;
using namespace stdexec;
using namespace std::chrono_literals;

TEST_CASE("simple", "timeout") {
  timeout(just(1), 10ms);
}
