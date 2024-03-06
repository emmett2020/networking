#include <catch2/catch_test_macros.hpp>
#include "utils/timeout.h"
#include <stdexec/execution.hpp>
#include <type_traits>

using namespace ex;
using namespace std::chrono_literals;

TEST_CASE("simple", "timeout") {
  timeout(just(1), 10ms);
}
