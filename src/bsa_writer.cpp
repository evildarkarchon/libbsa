#include <libbsa/bsa_writer.hpp>

namespace libbsa {

result<bsa_write_plan> plan_bsa_write(bsa_write_target target,
                                      std::span<const bsa_memory_entry> entries,
                                      bsa_write_options options)
{
    static_cast<void>(target);
    static_cast<void>(entries);
    static_cast<void>(options);
    return failure<bsa_write_plan>({error_code::unsupported_format, "BSA writer planning is not implemented"});
}

result<bsa_write_plan> plan_bsa_write_from_disk(bsa_write_target target,
                                                std::span<const bsa_disk_entry> entries,
                                                bsa_write_options options)
{
    static_cast<void>(target);
    static_cast<void>(entries);
    static_cast<void>(options);
    return failure<bsa_write_plan>({error_code::unsupported_format, "BSA disk writer planning is not implemented"});
}

result<void> finalize_bsa_write(const bsa_write_plan& plan, byte_sink& sink)
{
    static_cast<void>(plan);
    static_cast<void>(sink);
    return failure<void>({error_code::unsupported_format, "BSA writer finalization is not implemented"});
}

} // namespace libbsa
