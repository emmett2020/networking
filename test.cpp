#include <fmt/core.h>
#include <array>
#include <concepts>
#include <type_traits>

using tcp_socket = int;
using buffer = std::array<char, 10>;

struct Foo {
  Foo(const Foo &) = default;
  Foo(Foo &&) = delete;
  Foo &operator=(const Foo &) = default;
  Foo &operator=(Foo &&) = delete;
  constexpr virtual ~Foo() = default;

  constexpr virtual auto func() {
    return 1;
  }
};

template <typename T>
concept expected_t = requires(T &t) {
  { t.has_value() } -> std::convertible_to<bool>;
  { t.error() };
  { t.value() };
};

// return variant sender
template <class Func>
  requires(expected_t<std::invoke_result_t<Func, void>>)
ex::sender auto exeucte(Func &&func) noexcept {
  auto ret = func();
  using ret_t = ex::variant_sender<
    decltype(ex::just(ret.value())), //
    decltype(ex::just_error(ret.error()))>;
  if (ret.has_value()) {
    return ret_t(ex::just(ret.value()));
  } else {
    return ret_t(ex::just_error(ret.error()));
  }
}

execute([](response_t response) { return response.to_string(); });

just_variant([]() -> variant {


})

  ex::sender auto just_with_expected(Func &&func) {
}
