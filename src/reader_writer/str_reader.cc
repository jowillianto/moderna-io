module;
#include <algorithm>
#include <expected>
#include <iterator>
#include <ranges>
#include <string>
#include <vector>
export module moderna.io:str_reader;
import :basic_io;
import :is_file;
import :error;

namespace moderna::io {
  export struct str_reader {
    using result_type = std::string;
    const size_t read_count = -1;
    std::expected<std::string, fs_error> operator()(const is_basic_file auto &file) const {
      std::string buffer{};
      auto buffer_inserter = std::back_inserter(buffer);
      return read_from_file<2048>(file, read_count, buffer_inserter).transform([&]() {
        return std::move(buffer);
      });
    }
    template <size_t buffer_size>
    std::expected<void, fs_error> read_from_file(
      const is_basic_file auto &file, size_t amount_left, std::back_insert_iterator<std::string> &it
    ) const {
      auto read_res = basic_reader<buffer_size>{amount_left}(file);
      if (!read_res) {
        return std::unexpected{std::move(read_res.error())};
      }
      std::ranges::copy_n(read_res->buffer.begin(), read_res->size, it);
      if (read_res->size == buffer_size) {
        return read_from_file<std::min(buffer_size * 2, static_cast<size_t>(1048576))>(
          file, amount_left - buffer_size, it
        );
      } else {
        return {};
      }
    }
  };

  export struct delim_reader {
    using result_type = std::string;
    const char delimiter = '\n';
    std::expected<std::string, fs_error> operator()(const is_basic_file auto &file) const {
      std::expected<std::string, fs_error> buffer{std::string{}};
      auto buffer_inserter = std::back_inserter(buffer.value());
      while (true) {
        auto read_res = basic_reader<1>{1}(file);
        if (!read_res) {
          buffer = std::unexpected{std::move(read_res.error())};
          break;
        };
        if (read_res->size == 0 || read_res->buffer[0] == delimiter) {
          break;
        } else {
          std::ranges::copy_n(read_res->buffer.begin(), read_res->size, buffer_inserter);
        }
      }
      return buffer;
    }
  };

  export struct multi_delim_reader {
    using result_type = std::vector<std::string>;
    const char delimiter = '\n';
    std::expected<result_type, fs_error> operator()(const is_basic_file auto &file) const {
      std::expected<result_type, fs_error> read_delim_res{result_type{}};
      while (true) {
        auto read_res = delim_reader{delimiter}(file);
        if (!read_res) {
          read_delim_res = std::unexpected{std::move(read_res.error())};
          break;
        } else if (read_res->size() == 0) {
          break;
        } else {
          read_delim_res->emplace_back(std::move(read_res.value()));
        }
      }
      return read_delim_res;
    }
  };
  export struct csv_reader {
    using result_type = std::vector<std::vector<std::string>>;
    const char delimiter = ',';

    std::expected<result_type, fs_error> operator()(const is_basic_file auto &file) const {
      std::expected<result_type, fs_error> read_delim_res{result_type{}};
      while (true) {
        auto read_res = delim_reader{delimiter}(file);
        if (!read_res) {
          read_delim_res = std::unexpected{std::move(read_res.error())};
          break;
        } else if (read_res->size() == 0) {
          break;
        } else {
          read_delim_res->emplace_back(separate_by_delimiter(read_res.value()));
        }
      }
      return read_delim_res;
    }
    std::vector<std::string> separate_by_delimiter(std::string_view line) const {
      std::vector<std::string> delim_sep;
      for (const auto &entry : std::ranges::split_view{line, delimiter}) {
        delim_sep.emplace_back(std::string{entry.begin(), entry.end()});
      }
      return delim_sep;
    }
  };
}