#include "formats/bsa/tes3_bsa_layout.hpp"

#include <detail/payload_placement.hpp>

namespace libbsa::formats::bsa {

result<void> tes3_place_payloads(std::span<tes3_prepared_entry> entries) {
    // The payload area is seeded at zero because TES3 file records store
    // data-section-relative offsets and readers add the computed data section
    // start back. That relativity is a TES3 serialization fact rather than a
    // placement fact, so it stays with this family instead of moving into the
    // module.
    //
    // Sharing is a stated policy rather than absent code. TES3's reader exempts
    // no duplicate span, so a TES3 writer that ever shared a location could emit
    // an archive libbsa itself refuses to reopen; ADR-0002 records that as an
    // open question, and this policy value is what makes the exclusion greppable
    // instead of something a reader has to infer from code that is not there.
    detail::payload_placer placer{0U, detail::payload_sharing_policy::disabled, "TES3 BSA"};

    for (auto& entry : entries) {
        // TES3 deliberately does not adopt Stored Payload, so it offers a size
        // and takes only the cursor and the overflow arithmetic. A size-only
        // subject can never share under any policy — exact byte equality is the
        // sole authority for sharing (ADR-0001) and there are no bytes here to
        // supply that proof — so the narrowing key is inert and carries no
        // fingerprint.
        auto placed = placer.place(detail::payload_narrowing_key{entry.payload_size, 0U},
                                   detail::payload_placement_subject::of_size(entry.payload_size));
        if (!placed) {
            return placed.error();
        }
        entry.raw_offset = placed.value().offset;
    }

    return {};
}

}  // namespace libbsa::formats::bsa
