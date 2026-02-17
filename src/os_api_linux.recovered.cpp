#include "os_api.h"
#include <sys/mman.h>
#include <unistd.h>

os_api::OsResult os_api::reserve_address_space(std::size_t size,
                                               std::size_t alignment,
                                               MemSpan &out) {
  
  mmap(void *addr, size_t len, int prot, int flags, int fd, __off_t offset)
}
