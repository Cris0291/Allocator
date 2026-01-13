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
  };

public:
  SuperBlock() {
    std::size_t super_block_heaader_sz{sizeof(SuperBlockHeader)};

    // allocate usiing  os api for a span_size memory
    void *base
  }
};
