#include <moderna/test_lib.hpp>
#include <future>
#include <thread>
import moderna.test_lib;
import moderna.io;

namespace test_lib = moderna::test_lib;
namespace io = moderna::io;

MODERNA_SETUP(argc, argv) {
  test_lib::get_test_context().set_thread_count(1).set_time_unit(
    test_lib::test_time_unit::MILLI_SECONDS
  );
}
MODERNA_ADD_TEST(create_listener) {
  int port = test_lib::random_integer(40000, 60000);
  auto res = io::listen(port);
  test_lib::assert_expected(res);
  auto listener = std::move(res.value());
  test_lib::assert_equal(listener.host.port, port);
}
MODERNA_ADD_TEST(connect_to_listener) {
  int port = test_lib::random_integer(40000, 60000);
  auto res = io::listen(port);
  test_lib::assert_expected(res);
  auto listener = std::move(res.value());
  auto msg = test_lib::random_string(100);
  auto fut = std::async(
    std::launch::async,
    [](auto &&list, auto msg) {
      auto connection_res = io::accept(list);
      test_lib::assert_expected(connection_res);
      auto connection = std::move(connection_res.value());
      auto res = connection.file.write(msg);
      test_lib::assert_expected(res);
    },
    std::move(listener),
    msg
  );
  auto connection_res = io::connect("127.0.0.1", port);
  test_lib::assert_expected(connection_res);
  auto connection = std::move(connection_res.value());
  auto read_res = connection.file.read(msg.length());
  test_lib::assert_expected(read_res);
  test_lib::assert_equal(read_res.value(), msg);
  auto fut_res = fut.wait_for(std::chrono::seconds(1));
  if (fut_res != std::future_status::ready) {
    exit(1);
  }
}
MODERNA_ADD_TEST(connect_and_check_no_data) {
  int port = test_lib::random_integer(40000, 60000);
  auto res = io::listen(port);
  test_lib::assert_expected(res);
  auto listener = std::move(res.value());
  auto fut = std::async(
    std::launch::async,
    [](auto &&list) {
      auto connection_res = io::accept(list);
      test_lib::assert_expected(connection_res);
      auto connection = std::move(connection_res.value());
    },
    std::move(listener)
  );
  auto connection_res = io::connect("127.0.0.1", port);
  test_lib::assert_expected(connection_res);
  auto connection = std::move(connection_res.value());
  auto conn_have_data = connection.file.is_read_ready();
  test_lib::assert_expected(conn_have_data);
  test_lib::assert_false(conn_have_data.value());
  auto fut_res = fut.wait_for(std::chrono::seconds(1));
  if (fut_res != std::future_status::ready) {
    exit(1);
  }
}
MODERNA_ADD_TEST(connect_check_have_data) {
  int port = test_lib::random_integer(40000, 60000);
  auto res = io::listen(port);
  test_lib::assert_expected(res);
  auto listener = std::move(res.value());
  auto msg = test_lib::random_string(100);
  auto fut = std::async(
    std::launch::async,
    [](auto &&list, auto msg) {
      auto connection_res = io::accept(list);
      test_lib::assert_expected(connection_res);
      auto connection = std::move(connection_res.value());
      auto res = connection.file.write(msg);
      test_lib::assert_expected(res);
    },
    std::move(listener),
    msg
  );
  auto connection_res = io::connect("127.0.0.1", port);
  test_lib::assert_expected(connection_res);
  auto connection = std::move(connection_res.value());
  /*
    Wait for 1ms for data to be sent
  */
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  auto conn_have_data = connection.file.is_read_ready();
  test_lib::assert_expected(conn_have_data);
  test_lib::assert_true(conn_have_data.value());
  auto read_res = connection.file.read(msg.length());
  test_lib::assert_expected(read_res);
  test_lib::assert_equal(read_res.value(), msg);
  auto fut_res = fut.wait_for(std::chrono::seconds(1));
  if (fut_res != std::future_status::ready) {
    exit(1);
  }
}