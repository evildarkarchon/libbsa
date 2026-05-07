#include <libbsa/writer.hpp>

namespace libbsa {
namespace {

error writer_planning_not_implemented()
{
    return {error_code::unsupported_format, "writer planning is not implemented"};
}

error writer_finalization_not_implemented()
{
    return {error_code::unsupported_format, "writer finalization is not implemented"};
}

} // namespace

result<write_plan> plan_archive_write(const writer_target& target,
                                      std::span<const writer_entry> entries,
                                      writer_options options)
{
    (void)target;
    (void)entries;
    (void)options;
    return failure<write_plan>(writer_planning_not_implemented());
}

result<void> finalize_archive_write(const write_plan& plan, byte_sink& sink)
{
    (void)plan;
    (void)sink;
    return failure<void>(writer_finalization_not_implemented());
}

} // namespace libbsa
