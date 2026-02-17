#include "os_api.h"
#include <cstddef>
#include <cstdint>
#include <sys/mman.h>

std::size_t os_api::round_up_page(std::size_t size) {
  return (size + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1);
}

std::uintptr_t os_api::align_up(std::uintptr_t addr, std::size_t size) {
  return (addr + (size - 1)) & ~(size - 1);
}
os_api::OsResult os_api::reserve_address_space(std::size_t size,
                                               std::size_t alignment,
                                               MemSpan &out) {

  std::size_t rounded_size{round_up_page(size)};
  std::size_t mmap_len{rounded_size + (alignment - PAGE_SIZE)};

  void *mapped{mmap(nullptr, mmap_len, PROT_NONE, MAP_PRIVATE, -1, 0)};
  std::uintptr_t base{reinterpret_cast<std::uintptr_t>(mapped)};

  std::uintptr_t aligned_base{os_api::align_up(base, alignment)};
}
