#include <libbsa/result.hpp>

int main()
{
    libbsa::result<void> ok = libbsa::success();
    return ok.has_value() ? 0 : 1;
}
