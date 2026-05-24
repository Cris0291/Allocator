#pragma once

#include "extent_manager.h"
#include "lock_free_stack.h"
#include "map_size.h"
#include "os_api.h"
#include "super_block.h"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>

class Arena {
private:
  static constexpr std::size_t NUM_CLASSES{17};
  // Size of arena is 256 but allocated an extra page for header and metadata
  static constexpr std::size_t default_arena_size{260};
  struct ArenaHeader {
    std::uint32_t version;
    std::uint32_t magic;
    std::uint32_t flags;
    std::uint16_t arena_id;
    std::uint16_t owner_core;
    std::uint64_t total_usable_size;
    std::uintptr_t arena_stats_offset;
    std::uintptr_t super_block_health_offset;
    std::uintptr_t super_block_pool_offset;
    std::uintptr_t super_block_classes_offset;
    std::uintptr_t extent_manager_offset;
    std::uintptr_t extent_manager_pool_offset;
    std::uintptr_t lock_free_stack_offset;
    std::uintptr_t usable_region_offset;
    void *arena_base;
  };
  struct ArenaStats {
    std::atomic_uint_fast64_t alloc_count;
    std::atomic_uint_fast64_t free_count;
    std::atomic_uint_fast64_t bytes_allocated;
    std::atomic_uint_fast64_t bytes_free;
    std::atomic_uint_fast64_t bytes_in_use;
    std::atomic_uint_fast64_t bytes_wasted;
    std::uint8_t padding[(64 - ((6 * sizeof(uint64_t)) % 64)) % 64];
  };
  struct SuperBlockHealth {
    std::atomic_uint_fast64_t superblocks_active;
    std::atomic_uint_fast64_t superblocks_full;
    std::atomic_uint_fast64_t superblocks_created;
    std::atomic_uint_fast64_t superblocks_released;
    std::uint8_t padding[(64 - ((4 * sizeof(uint64_t)) % 64)) % 64];
  };
  struct SuperBlockPool {
    alignas(SuperBlock) unsigned char storage[4][sizeof(SuperBlock)];
    bool occupied[4]{};
  };
  struct SuperBlockPoolClasses {
    SuperBlock *super_block_pool_classes[NUM_CLASSES]{};
  };
  ExtentManager *extent_manager;
  struct ExtentManagerPool {
    ExtentManager::Entry entry_pool[64];
  };
  ArenaHeader *base;
  static std::uintptr_t align_up(std::uintptr_t x, std::size_t size);
  static ArenaHeader *init_header(void *base, std::uint32_t id,
                                  std::uint32_t core,
                                  std::uint32_t flags_config);
  static void init_arena_stats(ArenaHeader *header);
  void init_extent_manager(ArenaHeader *header);
  void init_super_block_pool();
  void init_super_block_classes();
  void init_super_block_health();
  void init_lock_free_stack();
  void *get_arena_stats();
  void *get_super_block_pool();
  void *get_super_block_health_address();
  void *get_super_block_classes();
  void *get_extent_manager_pool();
  void *get_lock_free_stack();
  bool has_arena_space(ArenaStats *arena_stats);
  bool has_super_block_space(ArenaStats *arena_stats);
  SuperBlock *alloc_super_block(std::size_t id);
  SuperBlock *find_super_block(std::uintptr_t ptr);
  SuperBlock *get_super_block_class_or_null(std::size_t id);
  ArenaStats *get_arena_stats_pointer();
  LockFreeStack *get_lock_free_stack_pointer();
  void set_bytes_allocated(std::size_t size, bool is_extent);
  void set_bytes_freed_extent(void *ptr);
  void set_bytes_freed_super_block(std::size_t size);

public:
  Arena(std::uint32_t id, std::uint32_t core, std::uint32_t flags_config);
  void *alloc(std::size_t id, std::size_t size);
  bool free(void *ptr);
  void free_remote(void *ptr);
  bool is_range_arena(std::uintptr_t ptr);
};
