#include "atomic_word_ops.h"
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

class SuperBlock {
  static constexpr std::size_t span_size{64 * 1024};
  static constexpr std::size_t SLOT_SIZE{16};
  static constexpr std::size_t HEADER_ALIGNMENT{
      16}; // Could be omitted we will see
  std::size_t total_number_slots;
  std::uint64_t *bitmap{};
  struct SuperBlockHeader {
    uint32_t class_id;
    uint32_t span_size;
    uint32_t slot_size;
    uint32_t n_slot;
    uint32_t free_count;
    uint32_t payload_ptr;
  };
  std::size_t bitmap_size_convergence_routine(std::size_t header_sz) {
    std::size_t N0 = std::floor(span_size - header_sz) / SLOT_SIZE;
    double N1{};
    while (1) {
      std::size_t bitmap_words{N0 / 64};
      std::size_t bitmap_size{8 * bitmap_words};
      N1 = std::floor((span_size - header_sz - bitmap_size)) / SLOT_SIZE;
      if (N0 == N1)
        break;
      N0 = N1;
    }
    return N0;
  }
  std::uintptr_t align_up(std::uintptr_t x, std::size_t size) {
    return (x + (size - 1) & ~(size - 1));
  }

public:
  SuperBlock(uint32_t class_id) {
    std::size_t super_block_header_sz{sizeof(SuperBlockHeader)};
    std::size_t header_aligned_sz{align_up(super_block_header_sz, 16)};
    // calulate payload size
    std::size_t slots{bitmap_size_convergence_routine(header_aligned_sz)};
    total_number_slots = slots;
    std::size_t bitmap_sz{(slots / 64) * 8};
    std::size_t payload_align_sz{align_up(header_aligned_sz + bitmap_sz, 16)};
    // allocate usiing  os api for a span_size memory - span_size
    void *raw;

    std::uintptr_t base{reinterpret_cast<uintptr_t>(raw)};
    SuperBlockHeader *header{reinterpret_cast<SuperBlockHeader *>(base)};
    header->class_id = class_id;
    header->span_size = span_size;
    header->slot_size = SLOT_SIZE;
    header->n_slot = slots;
    header->free_count = slots;
    header->payload_ptr = payload_align_sz;

    std::uintptr_t bitmap_ptr{base + header_aligned_sz};
    bitmap = &bitmap_ptr;
    // initialiize bitmap to 0's
    void *bitmap_vptr{reinterpret_cast<void *>(bitmap_ptr)};
    std::memset(bitmap_vptr, 0, bitmap_sz);
    // initialize padding of bitmap to 1 so that to avoid false chunks
    std::uintptr_t bitmap_padding{
        (payload_align_sz - header_aligned_sz - bitmap_sz)};
    void *bitmap_padding_vptr{reinterpret_cast<void *>(bitmap_padding)};
    // this is not right check  it later
    std::memset(bitmap_padding_vptr, 1, sizeof(bitmap_padding));
  }
  void allocate_atomic_span(std::uint64_t hint_word = 0) {
    std::uint64_t{atomic_word_load(&bitmap[hint_word])};
  }
};
