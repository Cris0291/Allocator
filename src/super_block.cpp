#include "atomic_word_ops.h"
#include <array>
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
  static constexpr std::size_t BIT_COUNT{64};
  std::array<std::uint64_t, BIT_COUNT> bitmasks{};
  std::size_t total_number_slots;
  std::uint64_t *bitmap{};
  struct SuperBlockHeader {
    uint32_t class_id;
    uint32_t span_size;
    uint32_t slot_size;
    uint32_t n_slot;
    std::atomic<std::uint32_t> free_count;
    std::uintptr_t payload_ptr;
  };
  SuperBlockHeader *super_block_header;
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
  std::array<std::uint64_t, BIT_COUNT> make_bitmasks() {
    std::array<std::uint64_t, BIT_COUNT> bitmasks_array{};
    for (int i{}; i < BIT_COUNT; i++) {
      std::uint64_t bitmask{1ULL << i};
      bitmasks_array[i] = bitmask;
    }

    return bitmasks_array;
  }

public:
  SuperBlock(uint32_t class_id) {
    bitmasks = make_bitmasks();
    std::size_t super_block_header_sz{sizeof(SuperBlockHeader)};
    std::size_t header_aligned_sz{align_up(super_block_header_sz, 16)};
    // calulate payload size
    std::size_t slots{bitmap_size_convergence_routine(header_aligned_sz)};
    total_number_slots = slots;
    std::size_t bitmap_sz{(slots / 64) * 8};
    std::uintptr_t payload_align_sz{
        align_up(header_aligned_sz + bitmap_sz, 16)};
    // allocate usiing  os api for a span_size memory - span_size
    void *raw;

    std::uintptr_t base{reinterpret_cast<uintptr_t>(raw)};
    SuperBlockHeader *header{reinterpret_cast<SuperBlockHeader *>(base)};
    super_block_header = header;
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
  void *allocate_atomic_span(std::size_t hint_word = 0) {
    std::size_t bitmap_words{total_number_slots / 64};
    if (hint_word > bitmap_words)
      hint_word = 0;

    while (hint_word <= bitmap_words) {
      std::uint64_t old_word = atomic_word_load(&bitmap[hint_word]);
      for (const auto &bitmask : bitmasks) {
        std::uint64_t free_mask = (~old_word) & bitmask;
        if (free_mask == 0)
          continue;
        int bit = __builtin_ctzll(free_mask);
        std::uint64_t single_mask = 1ULL << bit;

        std::uint64_t prev =
            atomic_word_fetch_or(&bitmap[hint_word], single_mask);
        if ((prev & single_mask) == 0) {
          super_block_header->free_count.fetch_sub(1);
          // TODO check if span is empty
          std::size_t slot_index = hint_word * 64 + bit;
          std::uintptr_t payload =
              super_block_header->payload_ptr + slot_index * SLOT_SIZE;
          // How should i return a hint also
          return reinterpret_cast<void *>(payload);
        } else {
          continue;
        }
      }
      ++hint_word;
    }
    // if reached this is full TODO
  }

  void free_atomic_span(void *payload) {
    std::uintptr_t p{reinterpret_cast<std::uintptr_t>(payload)};
    std::uintptr_t offset{super_block_header->payload_ptr - p};
    // TODO 0 <= offset < N * SLOT_SIZE also offset % SLOT_SIZE
    std::uintptr_t slot_index{offset / SLOT_SIZE};
    std::uintptr_t word_index{slot_index / 64};
    std::uintptr_t bit{slot_index % 64};
    std::uint64_t single_mask = 1ULL << bit;
    std::uint64_t word{atomic_word_load(&bitmap[word_index])};
    std::uint64_t prev{atomic_word_fetch_and(&word, ~single_mask)};
    // if((prev & single_mask) == 0) double free corruption hanlde it
    // TODO handle over addtion surpasing the available amount of slots
    super_block_header->free_count.fetch_add(1);
    return;
  }
};
