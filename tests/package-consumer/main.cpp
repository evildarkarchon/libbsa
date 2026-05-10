#include <libbsa/libbsa.hpp>

int main() {
  auto result = libbsa::validate_archive("consumer-smoke.bsa");
  if (result) {
    return 1;
  }

  return result.error().code == libbsa::error_code::io_error ? 0 : 1;
}
