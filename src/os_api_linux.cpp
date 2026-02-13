#include "os_api.h"
#include <cstddef>
#include <cstdint>
#include <sys/mman.h>

std::uintptr_t os_api::align_up(std::uintptr_t addr, std::size_t size) {
  return (addr + (size - 1)) & ~(size - 1);
}
os_api::OsResult os_api::reserve_address_space(std::size_t size,
                                               std::size_t alignment,
                                               MemSpan &out) {
  g mmap(void *addr, size_t len, int prot, int flags, int fd, __off_t offset)
}
