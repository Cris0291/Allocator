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

  std::uintptr_t prefix{aligned_base - base};
  // is this right to use mmaped also is normal size or rounded
  munmap(mapped, prefix);

  std::uintptr_t suffix{(base + mmap_len) - (aligned_base + size)};
  void *mapped_suffix{reinterpret_cast<void *>((aligned_base + size))};
  munmap(mapped_suffix, suffix);

  out.addr = (void *)aligned_base;
  out.size = aligned_base + size;
  out.committed = 0;
  out.numa_node = -1;
}

os_api::OsResult os_api::release_addresss_space(void *addr,
                                                std::size_t length) {
  munmap(addr, length);
}
