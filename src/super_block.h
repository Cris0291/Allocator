#pragma once

#include "atomic_word_ops.h"
#include "extent_manager.h"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>

class SuperBlock {
private:
  ExtentManager &manager;
  void *raw_base;
  std::size_t total_number_slots;
  std::uint64_t *bitmap{};
  std::size_t SLOT_SIZE{16};
  std::size_t bitmap_size_convergence_routine(std::size_t header_sz);
  std::uintptr_t align_up(std::uintptr_t x, std::size_t size);

public:
  static constexpr std::size_t span_size{64 * 1024};
  // the 8 is because the min slot size is 8 bytes then since i want thhe words
  // i divided the result by 64 plus one just in case i need an extra slot
  static constexpr std::size_t MAX_WORDS{span_size / 8 / 64 + 1};
  static constexpr std::uint32_t SUPER_BLOCK_MAGIC{0x5B10C000};
  struct SuperBlockHeader {
    uint32_t class_id;
    uint32_t span_size;
    uint32_t slot_size;
    uint32_t n_slot;
    std::uint32_t super_block_magic;
    std::atomic<std::uint32_t> free_count;
    std::uintptr_t payload_ptr;
    SuperBlock *super_block;
    std::uint32_t core;
  };
  SuperBlockHeader *super_block_header;
  SuperBlock *next{nullptr};
  inline static std::size_t get_super_block_size() { return span_size; };
  std::size_t get_slot_size();
  SuperBlock(uint32_t class_id, ExtentManager &extent_manager,
             std::size_t slot_size, std::uint32_t core);
  void *allocate_atomic_span(std::size_t hint_word);
  int allocate_atomic_block(void **out, int max_count = 64,
                            std::size_t hint_word = 0);
  void free_atomic_span(void *payload);
  void free_atomic_block(void **payloads, int max_count);
  bool is_full();
  bool is_empty();
  uint32_t free_count();
  void release();
  bool is_range(std::uintptr_t ptr);
  void set_own_pointer();
};
