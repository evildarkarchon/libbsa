#include "writer_harness_helpers.hpp"

#include <utility>

namespace libbsa::test {

writer_harness_fixture make_writer_harness_fixture(std::vector<writer_harness_entry_descriptor> entries)
{
    writer_harness_fixture fixture{};
    fixture.entries = std::move(entries);
    return fixture;
}

} // namespace libbsa::test
