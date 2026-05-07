#pragma once

#include "atomic_word_ops.h"
#include "extent_manager.h"
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>

class SuperBlock {
private:
  static constexpr std::size_t span_size{64 * 1024};
  ExtentManager &manager;
  void *raw_base;
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
  std::size_t SLOT_SIZE{16};
  SuperBlockHeader *super_block_header;
  std::size_t bitmap_size_convergence_routine(std::size_t header_sz);
  std::uintptr_t align_up(std::uintptr_t x, std::size_t size);

public:
  inline static std::size_t get_super_block_size() { return span_size; }
  SuperBlock(uint32_t class_id, ExtentManager &extent_manager,
             std::size_t slot_size);
  void *allocate_atomic_span(std::size_t hint_word);
  void free_atomic_span(void *payload);
  bool is_full();
  bool is_empty();
  uint32_t free_count();
  void release();
  bool is_range(std::uintptr_t ptr);
};
