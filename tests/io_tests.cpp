#include <catch2/catch_test_macros.hpp>

#include <libbsa/io.hpp>

#include <array>
#include <cstddef>
#include <span>

TEST_CASE("memory source reads bounded byte ranges", "[unit]")
{
    const std::array bytes{std::byte{0x10}, std::byte{0x20}, std::byte{0x30}, std::byte{0x40}};
    const libbsa::memory_source source{std::span<const std::byte>{bytes}};

    std::array<std::byte, 3> destination{};
    auto read = source.read_at(1, std::span<std::byte>{destination});

    REQUIRE(read.has_value());
    CHECK(source.size() == 4);
    CHECK(destination == std::array{std::byte{0x20}, std::byte{0x30}, std::byte{0x40}});
}

TEST_CASE("memory source rejects out of range reads", "[unit]")
{
    const std::array bytes{std::byte{0x10}, std::byte{0x20}, std::byte{0x30}, std::byte{0x40}};
    const libbsa::memory_source source{std::span<const std::byte>{bytes}};
    std::array<std::byte, 2> destination{};

    auto beyond_end = source.read_at(5, std::span<std::byte>{destination});
    auto crosses_end = source.read_at(3, std::span<std::byte>{destination});
    auto overflows = source.read_at(UINT64_MAX - 1, std::span<std::byte>{destination});

    REQUIRE_FALSE(beyond_end.has_value());
    CHECK(beyond_end.error().code == libbsa::error_code::io_failure);
    REQUIRE_FALSE(crosses_end.has_value());
    CHECK(crosses_end.error().code == libbsa::error_code::io_failure);
    REQUIRE_FALSE(overflows.has_value());
    CHECK(overflows.error().code == libbsa::error_code::io_failure);
}

TEST_CASE("memory sink appends bytes through byte sink contract", "[unit]")
{
    libbsa::memory_sink sink;
    libbsa::byte_sink& writer = sink;
    const std::array first{std::byte{0x01}, std::byte{0x02}};
    const std::array second{std::byte{0x03}};

    REQUIRE(writer.write(std::span<const std::byte>{first}).has_value());
    REQUIRE(writer.write(std::span<const std::byte>{second}).has_value());

    const std::vector expected{std::byte{0x01}, std::byte{0x02}, std::byte{0x03}};
    CHECK(sink.bytes() == expected);
}

TEST_CASE("public io helper types compile from libbsa io header", "[unit]")
{
    const std::array bytes{std::byte{0x7f}};
    const libbsa::memory_source source{std::span<const std::byte>{bytes}};
    libbsa::memory_sink sink;

    CHECK(source.size() == 1);
    CHECK(sink.bytes().empty());
}
