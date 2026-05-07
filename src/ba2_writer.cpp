#include <libbsa/ba2_writer.hpp>

#include <string_view>

namespace libbsa {
namespace {

result<ba2_write_plan> unsupported_ba2_plan(std::string_view message)
{
    return failure<ba2_write_plan>({error_code::unsupported_format, std::string{message}});
}

} // namespace

result<ba2_write_plan> plan_ba2_gnrl_write(ba2_write_target,
                                           std::span<const ba2_gnrl_memory_entry>,
                                           ba2_write_options)
{
    return unsupported_ba2_plan("BA2 GNRL writer planning is not implemented");
}

result<ba2_write_plan> plan_ba2_gnrl_write_from_disk(ba2_write_target,
                                                     std::span<const ba2_gnrl_disk_entry>,
                                                     ba2_write_options)
{
    return unsupported_ba2_plan("BA2 GNRL writer planning is not implemented");
}

result<ba2_write_plan> plan_ba2_dds_write(ba2_write_target,
                                          std::span<const ba2_dds_memory_entry>,
                                          ba2_write_options)
{
    return unsupported_ba2_plan("BA2 DDS writer planning is not implemented");
}

result<ba2_write_plan> plan_ba2_dds_write_from_disk(ba2_write_target,
                                                    std::span<const ba2_dds_disk_entry>,
                                                    ba2_write_options)
{
    return unsupported_ba2_plan("BA2 DDS writer planning is not implemented");
}

result<void> finalize_ba2_write(const ba2_write_plan&, byte_sink&)
{
    return failure<void>({error_code::unsupported_format, "BA2 writer finalization is not implemented"});
}

} // namespace libbsa
