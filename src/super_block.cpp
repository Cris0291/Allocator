#include <cstddef>
#include <cstdint>

class SuperBlock {
  static constexpr std::size_t span_size{64 * 1024};
  static constexpr std::size_t slot_size{16};
  static constexpr std::size_t N{span_size / slot_size};
  struct SuperBlockHeader {
    uint32_t class_id;
    uint32_t span_size;
    uint32_t slot_size;
    uint32_t n_slot;
    uint32_t free_count;
  };

public:
  SuperBlock(uint32_t class_id) {
    std::size_t super_block_heaader_sz{sizeof(SuperBlockHeader)};

    // allocate usiing  os api for a span_size memory
    void *raw;
    std::uintptr_t base{reinterpret_cast<uintptr_t>(raw)};
    SuperBlockHeader *header{reinterpret_cast<uintptr_t>(base)};
    header->class_id = class_id;
    header->span_size = span_size;
    header->slot_size = slot_size;
    header->n_slot = N;
    header->free_count = N;
  }
};
