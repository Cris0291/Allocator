#include <cstddef>
#include <cstdint>
#include <utility>
class HugeAlloc final {
private:
  HugeAlloc() = delete;
  struct HugeHeader {
    std::size_t size;
  };
  static void *include_header(std::uintptr_t base_header, std::size_t size) {
    std::uintptr_t base{base_header + sizeof(HugeHeader)};
    HugeHeader *header{reinterpret_cast<HugeHeader *>(base_header)};
    header->size = size;
    return reinterpret_cast<void *>(base);
  }

  static std::pair<void *, std::size_t> get_header_size(void *base) {
    std::uintptr_t base_addr{reinterpret_cast<std::uintptr_t>(base)};
    std::uintptr_t base_header{base_addr - sizeof(HugeHeader)};
    HugeHeader *header{reinterpret_cast<HugeHeader *>(base_header)};
    return {reinterpret_cast<void *>(base_header), header->size};
  }

public:
  static void *alloc(std::size_t size, std::size_t alignment);
  static void free(void *raw);
};
