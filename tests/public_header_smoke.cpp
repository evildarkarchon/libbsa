#include <libbsa/archive.hpp>

#include <cstdint>
#include <type_traits>

int main()
{
    static_assert(std::is_enum_v<libbsa::ArchiveFormat>);
    static_assert(std::is_enum_v<libbsa::ErrorCode>);
    static_assert(!std::is_default_constructible_v<libbsa::ArchiveReader>);

    libbsa::ArchiveMetadata metadata{};
    metadata.version = 0x67;
    return metadata.version == 0x67 ? 0 : 1;
}
