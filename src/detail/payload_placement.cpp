#include <detail/payload_placement.hpp>

#include <stdexcept>
#include <utility>

namespace libbsa::detail {

payload_placement_subject payload_placement_subject::of_payload(stored_payload payload) noexcept {
    return payload_placement_subject{
        std::variant<stored_payload, std::uint64_t>{std::move(payload)}};
}

payload_placement_subject payload_placement_subject::of_size(std::uint64_t size) noexcept {
    return payload_placement_subject{std::variant<stored_payload, std::uint64_t>{size}};
}

payload_placement_subject::payload_placement_subject(
    std::variant<stored_payload, std::uint64_t> held) noexcept
    : held_(std::move(held)) {}

const stored_payload* payload_placement_subject::payload() const noexcept {
    return std::get_if<stored_payload>(&held_);
}

std::uint64_t payload_placement_subject::size() const noexcept {
    const auto* offered = payload();
    return offered != nullptr ? offered->size() : std::get<std::uint64_t>(held_);
}

bool payload_placement_subject::has_payload() const noexcept { return payload() != nullptr; }

stored_payload payload_placement_subject::take_payload() {
    auto* offered = std::get_if<stored_payload>(&held_);
    if (offered == nullptr) {
        throw std::logic_error("libbsa::detail::payload_placement_subject holds no payload");
    }
    return std::move(*offered);
}

payload_placer::payload_placer(std::uint64_t base_offset, payload_sharing_policy sharing_policy,
                               std::string_view diagnostic_label)
    : engine_(base_offset, sharing_policy, diagnostic_label) {}

result<payload_placement> payload_placer::place(const payload_narrowing_key& key,
                                                payload_placement_subject subject) {
    const auto candidate_is_eligible = [](const placed_payload&) noexcept { return true; };
    const auto accept = [](placed_payload placed, bool) { return placed; };
    return engine_.place(key, std::move(subject), candidate_is_eligible, accept);
}

std::uint64_t payload_placer::cursor() const noexcept { return engine_.cursor(); }

std::size_t payload_placer::placed_payload_count() const noexcept {
    return engine_.placed_payload_count();
}

std::vector<placed_payload> payload_placer::release() && { return std::move(engine_).release(); }

}  // namespace libbsa::detail
