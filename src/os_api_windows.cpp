#include "os_api.h"
#include <cstddef>
#include <windows.h>

std::size_t os_api::round_up_page(std::size_t size) {
  // need to initialiize page size as a windows  constant
  return (size + (4096 - 1)) & ~(4096 - 1);
}

os_api::OsResult os_api::reserve_address_space(std::size_t size,
                                               std::size_t alignment,
                                               MemSpan &out) {
  std::size_t rounded_to_page_size{round_up_page(size)};
  std::size_t size_with_alignment{rounded_to_page_size + (alignment - 4096)};
}
