#pragma once

#include <cstddef>
#include <vector>

namespace libbsa::tests {

/// Two byte sequences that are distinct but share one FNV-1a fingerprint.
struct fingerprint_collision_fixture {
    std::vector<std::byte> distinct_a;
    std::vector<std::byte> distinct_b;
};

/// Manufactures a genuine 64-bit FNV-1a collision for Payload Placement tests.
///
/// Both sequences are eight bytes long and hash to the same value under the
/// `stored_payload` fingerprint, so a placer that trusted the fingerprint alone
/// would share one location between them. Byte equality is the only authority
/// for sharing (ADR-0001), so every family and the shared Payload Placement
/// module must keep them apart.
///
/// The pair is shared rather than duplicated per test file so that a future
/// change to the fingerprint function invalidates exactly one definition.
/// Regenerate both sequences together if `stored_payload::fingerprint` ever
/// changes; a stale pair would silently stop testing collision handling.
inline fingerprint_collision_fixture fnv1a_fingerprint_collision() {
    return fingerprint_collision_fixture{
        .distinct_a =
            {
                std::byte{0x1D},
                std::byte{0x50},
                std::byte{0xD0},
                std::byte{0x37},
                std::byte{0x4E},
                std::byte{0xC6},
                std::byte{0xF8},
                std::byte{0x00},
            },
        .distinct_b =
            {
                std::byte{0xB1},
                std::byte{0xFB},
                std::byte{0x78},
                std::byte{0x97},
                std::byte{0xB8},
                std::byte{0x62},
                std::byte{0x20},
                std::byte{0x25},
            },
    };
}

}  // namespace libbsa::tests
