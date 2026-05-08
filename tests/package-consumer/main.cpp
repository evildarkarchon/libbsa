#include <libbsa/libbsa.hpp>

int main() {
  auto result = libbsa::archive_reader::open("consumer-smoke.bsa");
  if (result) {
    return 1;
  }

  return result.error().code == libbsa::error_code::unsupported ? 0 : 1;
}
