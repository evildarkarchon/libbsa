#include <libbsa/io.hpp>

#include <algorithm>
#include <limits>

namespace libbsa {

memory_source::memory_source(std::span<const std::byte> bytes) noexcept : bytes_(bytes) {}

std::uint64_t memory_source::size() const noexcept
{
    return static_cast<std::uint64_t>(bytes_.size());
}

result<void> memory_source::read_at(std::uint64_t offset, std::span<std::byte> destination) const
{
    const auto available = size();
    const auto requested = static_cast<std::uint64_t>(destination.size());
    if (offset > available || requested > available - offset) {
        return failure<void>({error_code::io_failure, "read range exceeds source size"});
    }

    const auto begin = bytes_.begin() + static_cast<std::ptrdiff_t>(offset);
    std::copy_n(begin, destination.size(), destination.begin());
    return success();
}

result<void> memory_sink::write(std::span<const std::byte> bytes)
{
    bytes_.insert(bytes_.end(), bytes.begin(), bytes.end());
    return success();
}

const std::vector<std::byte>& memory_sink::bytes() const noexcept
{
    return bytes_;
}

} // namespace libbsa
