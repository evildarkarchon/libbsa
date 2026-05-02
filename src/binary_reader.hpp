#pragma once

#include <libbsa/archive.hpp>

#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace libbsa::detail {

class BinaryReader {
public:
    explicit BinaryReader(const std::vector<std::uint8_t>& bytes) noexcept
        : bytes_(bytes)
    {
    }

    [[nodiscard]] std::size_t position() const noexcept
    {
        return position_;
    }

    [[nodiscard]] std::size_t size() const noexcept
    {
        return bytes_.size();
    }

    [[nodiscard]] bool seek(std::size_t position) noexcept
    {
        if (position > bytes_.size()) {
            return false;
        }
        position_ = position;
        return true;
    }

    [[nodiscard]] bool skip(std::size_t count) noexcept
    {
        return seek(position_ + count);
    }

    [[nodiscard]] Result<std::uint8_t> read_u8()
    {
        if (remaining() < 1U) {
            return malformed("unexpected end of archive while reading byte");
        }
        return bytes_[position_++];
    }

    [[nodiscard]] Result<std::uint32_t> read_u32()
    {
        if (remaining() < 4U) {
            return malformed("unexpected end of archive while reading uint32");
        }

        std::uint32_t value = 0;
        for (int shift = 0; shift < 32; shift += 8) {
            value |= static_cast<std::uint32_t>(bytes_[position_++]) << shift;
        }
        return value;
    }

    [[nodiscard]] Result<std::uint64_t> read_u64()
    {
        if (remaining() < 8U) {
            return malformed("unexpected end of archive while reading uint64");
        }

        std::uint64_t value = 0;
        for (int shift = 0; shift < 64; shift += 8) {
            value |= static_cast<std::uint64_t>(bytes_[position_++]) << shift;
        }
        return value;
    }

    [[nodiscard]] Result<std::string> read_string_len(bool terminated = true)
    {
        auto length_result = read_u8();
        if (!length_result) {
            return length_result.error();
        }

        const auto length = length_result.value();
        if (remaining() < length) {
            return malformed("length-prefixed string extends past archive bounds");
        }

        std::string value(
            reinterpret_cast<const char*>(bytes_.data() + position_),
            reinterpret_cast<const char*>(bytes_.data() + position_ + length));
        position_ += length;
        if (terminated && !value.empty()) {
            value.pop_back();
        }
        return value;
    }

    [[nodiscard]] Result<std::string> read_string_term()
    {
        const auto start = position_;
        while (position_ < bytes_.size() && bytes_[position_] != 0U) {
            ++position_;
        }
        if (position_ >= bytes_.size()) {
            return malformed("unterminated string extends past archive bounds");
        }

        std::string value(
            reinterpret_cast<const char*>(bytes_.data() + start),
            reinterpret_cast<const char*>(bytes_.data() + position_));
        ++position_;
        return value;
    }

private:
    [[nodiscard]] std::size_t remaining() const noexcept
    {
        return bytes_.size() - position_;
    }

    [[nodiscard]] Error malformed(std::string message) const
    {
        return {ErrorCode::malformed_archive, std::move(message)};
    }

    const std::vector<std::uint8_t>& bytes_;
    std::size_t position_ = 0;
};

} // namespace libbsa::detail
