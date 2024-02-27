#include <chrono>
using namespace chrono_literal;

// move to sio
static constexpr auto unlimited_time = std::chrono::seconds::max();

struct recv_option {
  std::chrono::hour keepalive_time{1};
  std::chrono::seconds recv_time{120};
};

struct recv_handle {
  tcp_socket socket;
  std::array<std::byte, 8192> buffer;
  uint32_t buffer_len{0};
  http::request request;
  http::parser parser(request);
  recv_option recv_opt;
  recv_metric recv_metrics;
  std::chrono::seconds remaining{unlimited_time};
};

// unlimited == uint64_t(-1)
// 0 == immediately set error
// other: real time

enum class io_mode {
  input,
  output,
  both,
};

// if dur = 0, immediately set_error
// if dur > 0, set_value if io mode triggerred duration `time_ms`.
//                 set_error otherwise.
//                 set_stopped if canceled,
// if dur == max, means unlimited, always wait for io mode to be triggerred unless canceled.
//  return the io_mode which be triggerred and real wait time.
ex::sender auto wait_io_for(io_mode mode, tcp_socket socket, std::chrono::seconds dur);

void initialize_handle(recv_handle& handle) {
  if (handle.recv_opt.keepalive_time != unlimited_time) {
    handle.remaining_time = handle.recv_opt.keepalive_time;
  } else {
    handle.remaining_time = handle.recv_opt.recv_time;
    handle.start_recv_time = std::chrono::steady_clock::now();
  }
}

using Variant =
  variant_sender<decltype(ex::let_error(http::error::recv_timeout)), decltype(just(0))>;

Variant update_handle(std::size_t recv_size, socket_recv_handle& handle) {
  handle.metric.total_recv_sie += recv_size;
  handle.metric.io_time += duration;
  handle.io_time += now() - handle.start_recv_time;
  handle.remaining_time = handle.recv_opt.recv_time - handle.metric.io_time;
}

ex::sender auto parse(recv_handle& handle) {
  // some parse work
  return parse_is_done; // bool
}

using result_variant = variant_sender<>;

result_variant final_result(tcp_recv_handle& handle) {
  if (parser.state == Parser::State::completed) {
    return ex::just{handle.request, handle.metrics};
  } else {
    return ex::let_error(detailed_error(parser.state));
  }
}

std::chrono::seconds update_time(recv_handle& handle) {
  handle.start_recv_time = std::chrono::steady_clock::now();
  return handle.remaining_time;
}

ex::sender auto recv_some(tcp_recv_handle& handle) {
  handle.start_recv_time = std::chrono::steady_clock::now();
  return ex::read_some(handle.socket, handle.buffer, handle.remaining_time) // overload
       | ex::let_value([&](size_t recv_size) {
           if (recv_size == 0) {
             return {ex::just_error(http::error::end_of_file)}; // variant
           }
           handle.io_time += now() - handle.start_recv_time;
           update_handle(sz, handle);
           return just(handle);
         });
}

// WARN: It seems that we currently cann't make a beautiful wapper to pass args to cpo.
//       Use lambda instead.

// for the time operation we have three chioces:
//      1. within_for
//      2. measure_time_start / measure_time_stop / measure_time_curruent
//      3. measure_time

// sender adaptor:
//    within_for() // execute previous sender within timeout, return real time and previous sender value
// usage:
//      return read_some(socket, buffer)
//           | within_for(remaining_time) // blocks until previous operation finished or timeout
//           | ex::let_value([](start_timepoint, stop_timepoint, elapsed_time, recv_size){
//                metric.start_timepoint = start_timepoint;
//                metric.end_timepoint = stop_timepoint;
//                metric.io_elapsed_time += elapsed_time;
//                metric.total_recv_sie += recv_size;
//              })
//           | ex::let_value(parse_request)

// try to use query to implement this function?
// three sender adaptors
// measure_time_start()
// measure_time_curruent() // get current elapsed time against start. This could be called multiply times.
// measure_time_stop()     // This will close the closest measuring operation.
// usage:
//  return just(1)
//       | measure_time_start() // pass previous sender values to next sender
//       | ex::let_value([](int n){
//          assert(n == 1);
//          read_some(socket, buffer)
//       })
//       | measure_time_stop() // pass the real duration between start and stop, and previous sender values
//       | ex::let_value([](std::chrono::duration elapsed_time, std::size_t recv_size){
//
//         });

auto update_wait_time(elapsed_time, _, remaining_time) {
}

// could be use as:
//        return measure_time_start()
//             | wait_io_for(io_mode::recv, socket, remaining_time)
//             | measure_time_curruent(update_wait_time)
//             | read_some(socket, buffer)

// sender adaptor
// measure_time() // execute previous sender and return start-timepoint, stop-timepoint, and pass down previous sender's value
// usage:
//    return wait_io_for(io_mode, socket, timeout)
//         | read_some(socket, buffer)
//         | measure_time()
//         | next_operation()

// what we currently could implement
/*
 *  initialize_handle();
 *  return update_start_time(handle)
 *         | ex::let_value([&] (duration timeout) {
 *            return ex::read_some(handle.socket, handle.buffer, timeout);
 *           })
 *         | ex::let_value([&] (std::size_t recv_size) {
 *                 // check size
 *                 // update handle
 *            })
 *         | ex::then(parse)
 *         | ex::repeat_effect_until()
 *         | ex::let_value(final_result);
 *
*/

// what style we want to implement
/*
 *  initialize_handle();
 *  return mark_recv_time()
 *         | ex::recv_some_timeout(handle.socket, handle.buffer, handle.remaining_time)
 *         | ex::then(ex::bind(update_recv_metrics, _1, handle))
 *         | ex::then(ex::bind(parse, handle.buffer, handle.unparsed_size))
 *         | ex::repeat_effect_until()
 *         | ex::let_value(final_result);
 *
*/

// Results generated by previous sender should be reorder to current sender.
/*
 * two_results()
 * | then(adaptor(some_func(a, _1,  b, _2)))  // _1 & _2 is generated by two_results()
 *
 *
 */


// implement a pack? usage:
// ex::let_value([](){
//    return pack(int{5}, read_some(socket, buffer));
//    })
// | then([](int, size_t){})
