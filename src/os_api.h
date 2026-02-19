#include <cstddef>
#include <cstdint>
#include <unistd.h>
namespace os_api {

const auto PAGE_SIZE = sysconf(_SC_PAGESIZE);

enum class OsResult {
  Success,
  OutOfMemory,
  NotSupported,
  InvalidArgument,
  PermissionDenied,
  UnkownError
};

struct MemSpan {
  void *addr;
  std::size_t size;
  std::size_t committed;
  int numa_node;
};

enum class Advice {
  Normal,
  WillNeed,
  DontNeed,
  HugePage,
  NoHugePage,
  Randdomize
};

void os_init();
std::size_t os_page_size();
std::size_t os_large_page_size();
bool os_supports_huge_pages();

OsResult reserve_address_space(std::size_t size, std::size_t alignment,
                               MemSpan &out);
OsResult commit_memory(void *addr, std::size_t length);
OsResult decommit_memory(void *addr, std::size_t length);
OsResult release_addresss_space(void *addr, std::size_t length);
OsResult reserve_with_hugh_pages(std::size_t size, std::size_t alignment,
                                 MemSpan &out);
OsResult set_protection(void *addr, std::size_t lenght, int prop_flags);
OsResult advice(void *addr, std::size_t length, Advice advice);

std::size_t query_commited_memory(void *addr, std::size_t size);
std::uintptr_t align_up(std::uintptr_t addr, std::size_t size);
std::size_t round_up_page(std::size_t size);

} // namespace os_api
