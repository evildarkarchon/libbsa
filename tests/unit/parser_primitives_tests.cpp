#include <catch2/catch_test_macros.hpp>

#include <detail/parser_primitives.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <span>
#include <string>
#include <unordered_set>
#include <vector>

namespace
{

  std::size_t impossible_string_size()
  {
    const auto max_size = std::string{}.max_size();
    if (max_size == std::numeric_limits<std::size_t>::max())
    {
      SKIP("string max_size cannot be overflowed on this standard library");
    }
    return max_size + 1U;
  }

  template <typename T>
  std::size_t impossible_vector_capacity()
  {
    const auto max_size = std::vector<T>{}.max_size();
    if (max_size == std::numeric_limits<std::size_t>::max())
    {
      SKIP("vector max_size cannot be overflowed on this standard library");
    }
    return max_size + 1U;
  }

  template <typename T>
  std::size_t impossible_set_capacity()
  {
    const auto max_size = std::unordered_set<T>{}.max_size();
    if (max_size == std::numeric_limits<std::size_t>::max())
    {
      SKIP("unordered_set max_size cannot be overflowed on this standard library");
    }
    return max_size + 1U;
  }

  class temp_file_cleanup
  {
  public:
    explicit temp_file_cleanup(std::filesystem::path path) : path_(std::move(path)) {}

    ~temp_file_cleanup()
    {
      std::error_code ignored;
      std::filesystem::remove(path_, ignored);
    }

  private:
    std::filesystem::path path_;
  };

} // namespace

TEST_CASE("parser_primitives validate checked arithmetic", "[unit][parser_primitives]")
{
  std::size_t total = 0;

  REQUIRE(libbsa::detail::multiply_fits(4U, 8U, total));
  REQUIRE(total == 32U);

  REQUIRE_FALSE(libbsa::detail::multiply_fits(std::numeric_limits<std::uint32_t>::max(),
                                              std::numeric_limits<std::size_t>::max(),
                                              total));

  REQUIRE(libbsa::detail::add_fits(12U, 30U, total));
  REQUIRE(total == 42U);

  REQUIRE_FALSE(libbsa::detail::add_fits(std::numeric_limits<std::size_t>::max(), 1U, total));

  std::uint64_t total64 = 0;
  REQUIRE(libbsa::detail::add_fits_u64(12U, 30U, total64));
  REQUIRE(total64 == 42U);

  // BA2 host-file parsing uses this contract for FileTableOffset plus parsed filename-table bytes; a full fixture
  // cannot usually seek to offsets high enough to overflow UInt64 before stream limits reject it.
  constexpr auto nearly_max_u64 = std::numeric_limits<std::uint64_t>::max() - 1U;
  REQUIRE(libbsa::detail::add_fits_u64(nearly_max_u64, 1U, total64));
  REQUIRE(total64 == std::numeric_limits<std::uint64_t>::max());
  REQUIRE_FALSE(libbsa::detail::add_fits_u64(nearly_max_u64, 2U, total64));

  REQUIRE_FALSE(libbsa::detail::add_fits_u64(std::numeric_limits<std::uint64_t>::max(), 1U, total64));
}

TEST_CASE("parser_primitives reject spans outside archive bounds", "[unit][parser_primitives][malformed]")
{
  REQUIRE(libbsa::detail::span_fits(2U, 3U, 5U));
  REQUIRE(libbsa::detail::span_fits(5U, 0U, 5U));
  REQUIRE_FALSE(libbsa::detail::span_fits(3U, 3U, 5U));
  REQUIRE_FALSE(libbsa::detail::span_fits(std::numeric_limits<std::size_t>::max(), 1U, 5U));

  REQUIRE(libbsa::detail::span_fits_u64(2U, 3U, 5U));
  REQUIRE(libbsa::detail::span_fits_u64(5U, 0U, 5U));
  REQUIRE_FALSE(libbsa::detail::span_fits_u64(3U, 3U, 5U));
  REQUIRE_FALSE(libbsa::detail::span_fits_u64(std::numeric_limits<std::uint64_t>::max(), 1U, 5U));
}

TEST_CASE("parser_primitives enforce metadata count limits", "[unit][parser_primitives][malformed]")
{
  auto accepted = libbsa::detail::validate_metadata_count(libbsa::detail::metadata_entry_count_limit,
                                                          libbsa::detail::metadata_entry_count_limit,
                                                          "test metadata count");
  REQUIRE(accepted.has_value());

  auto rejected = libbsa::detail::validate_metadata_count(libbsa::detail::metadata_entry_count_limit + 1U,
                                                          libbsa::detail::metadata_entry_count_limit,
                                                          "test metadata count");
  REQUIRE_FALSE(rejected.has_value());
  REQUIRE(rejected.error().code == libbsa::error_code::format_error);
}

TEST_CASE("parser_primitives translate typed metadata reserve failures", "[unit][parser_primitives][allocation]")
{
  std::vector<std::uint64_t> values;
  auto vector_reserved = libbsa::detail::reserve_metadata_vector(values, 4U, "test metadata vector");
  REQUIRE(vector_reserved.has_value());
  REQUIRE(values.capacity() >= 4U);

  auto impossible_vector = libbsa::detail::reserve_metadata_vector(values,
                                                                   impossible_vector_capacity<std::uint64_t>(),
                                                                   "test metadata vector");
  REQUIRE_FALSE(impossible_vector.has_value());
  REQUIRE(impossible_vector.error().code == libbsa::error_code::format_error);

  std::unordered_set<std::uint64_t> set;
  auto set_reserved = libbsa::detail::reserve_metadata_set(set, 4U, "test metadata set");
  REQUIRE(set_reserved.has_value());

  auto impossible_set = libbsa::detail::reserve_metadata_set(set,
                                                             impossible_set_capacity<std::uint64_t>(),
                                                             "test metadata set");
  REQUIRE_FALSE(impossible_set.has_value());
  REQUIRE(impossible_set.error().code == libbsa::error_code::format_error);
}

TEST_CASE("parser_primitives read exact bounded file spans", "[unit][parser_primitives]")
{
  const auto temp_path = std::filesystem::temp_directory_path() / "libbsa_parser_primitives_exact_read.bin";
  temp_file_cleanup cleanup{temp_path};

  {
    std::ofstream output{temp_path, std::ios::binary | std::ios::trunc};
    REQUIRE(output);
    const char bytes[] = {'a', 'b', 'c', 'd', 'e'};
    output.write(bytes, static_cast<std::streamsize>(sizeof(bytes)));
    REQUIRE(output);
  }

  std::ifstream input{temp_path, std::ios::binary};
  REQUIRE(input);

  auto middle = libbsa::detail::read_file_bytes_at(input, 1U, 3U, "test parser span");
  REQUIRE(middle);
  REQUIRE(middle.value().size() == 3U);
  CHECK(middle.value()[0] == std::byte{'b'});
  CHECK(middle.value()[2] == std::byte{'d'});

  auto truncated = libbsa::detail::read_file_bytes_at(input, 4U, 2U, "test parser span");
  REQUIRE_FALSE(truncated);
  REQUIRE(truncated.error().code == libbsa::error_code::format_error);
}

TEST_CASE("parser_primitives reject stream limits before seeking or allocating", "[unit][parser_primitives][malformed]")
{
  std::ifstream input;

  auto bad_offset = libbsa::detail::read_file_bytes_at(
      input,
      static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max()) + 1U,
      0U,
      "test parser span");
  REQUIRE_FALSE(bad_offset);
  REQUIRE(bad_offset.error().code == libbsa::error_code::format_error);

  auto bad_size = libbsa::detail::read_file_bytes_at(input,
                                                     0U,
                                                     static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max()) + 1U,
                                                     "test parser span");
  REQUIRE_FALSE(bad_size);
  REQUIRE(bad_size.error().code == libbsa::error_code::format_error);
}

TEST_CASE("parser_primitives materialize archive strings through result errors",
          "[unit][parser_primitives][allocation]")
{
  const std::vector<std::byte> bytes{std::byte{'M'}, std::byte{'e'}, std::byte{'s'}, std::byte{'h'}};

  auto value = libbsa::detail::archive_string_from_bytes(bytes, "test archive string");
  REQUIRE(value);
  REQUIRE(value.value() == "Mesh");

  const std::byte source{};
  auto impossible = libbsa::detail::archive_string_from_bytes(
      std::span<const std::byte>{&source, impossible_string_size()},
      "test archive string");
  REQUIRE_FALSE(impossible);
  REQUIRE(impossible.error().code == libbsa::error_code::format_error);
}

TEST_CASE("parser_primitives normalize display separators only", "[unit][parser_primitives]")
{
  std::string path = R"(Meshes\Actors/FaceGen\foo.nif)";

  libbsa::detail::normalize_display_separators(path);

  REQUIRE(path == "Meshes/Actors/FaceGen/foo.nif");
}
