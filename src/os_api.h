#include <cstddef>
namespace os_api {
enum class OsResult {
  Ok,
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

} // namespace os_api
