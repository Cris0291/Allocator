#pragma once

#include "extent_manager.h"
#include "lock_free_stack.h"
#include "map_size.h"
#include "os_api.h"
#include "super_block.h"
#include "tcache.h"
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
  struct SuperBlockList {
    SuperBlock *list[NUM_CLASSES]{};
  };
  struct ArenaHeader {
    std::uint32_t version;
    std::uint32_t magic;
    std::uint32_t flags;
    std::uint16_t arena_id;
    std::uint16_t owner_core;
    std::uint32_t super_block_count;
    std::uint32_t middle_chunk_count;
    std::uintptr_t arena_stats_offset;
    std::uintptr_t super_block_active_offset;
    std::uintptr_t super_block_partial_offset;
    std::uintptr_t super_block_full_offset;
    std::uintptr_t lock_free_stack_offset;
    SuperBlockList *active;
    SuperBlockList *partial;
    SuperBlockList *full;
    void *arena_base;
  };
  struct SuperBlockHeader {
    std::uint16_t super_block_header_id;
    std::uint16_t owner_core;
    std::uint64_t total_used_size;
    std::uint64_t total_usable_size;
    std::uintptr_t super_block_pool_offset;
    std::uintptr_t super_block_classes_offset;
    std::uintptr_t extent_manager_offset;
    std::uintptr_t extent_manager_pool_offset;
    std::uintptr_t super_block_chunk_offset;
    std::uintptr_t usable_region_offset;
    void *base;
  };
  struct MediumChunkHeader {
    std::uint16_t medium_chunk_header_id;
    std::uint16_t owner_core;
    std::uint64_t total_used_size;
    std::uint64_t total_usable_size;
    std::uintptr_t extent_manager_offset;
    std::uintptr_t extent_manager_pool_offset;
    std::uintptr_t medium_chunk_offset;
    std::uintptr_t usable_region_offset;
    void *base;
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
  struct ExtentManagerPool {
    ExtentManager::Entry entry_pool[64];
  };
  struct SuperBlockChunk {
    SuperBlockHeader *header{nullptr};
    ExtentManager *extent_manager{nullptr};
    SuperBlockChunk *next{nullptr};
  };
  struct MediumChunk {
    MediumChunkHeader *header{nullptr};
    ExtentManager *extent_manager{nullptr};
    MediumChunk *next{nullptr};
  };
  ArenaHeader *arena_header;
  SuperBlockChunk *head_super_block{nullptr};
  MediumChunk *head_medium_chunk{nullptr};
  static std::uintptr_t align_up(std::uintptr_t x, std::size_t size);
  static ArenaHeader *init_arena_header(void *base, std::uint32_t id,
                                        std::uint32_t core,
                                        std::uint32_t flags_config);
  static SuperBlockHeader *init_super_block_header(void *super_block_chunk_base,
                                                   std::uint32_t id,
                                                   std::uint32_t core);
  static MediumChunkHeader *init_medium_header(void *medium_chunk_base,
                                               std::uint32_t id,
                                               std::uint32_t core);
  static void init_arena_stats(ArenaHeader *header);
  ExtentManager *init_extent_manager(void *base,
                                     std::uintptr_t extent_manager_offset,
                                     std::uintptr_t extent_manager_pool_offset,
                                     std::uintptr_t usable_region_offset,
                                     std::uint64_t total_usable_size);
  void init_super_block_pool(SuperBlockHeader *header);
  void init_super_block_classes(SuperBlockHeader *header);
  SuperBlockList *init_super_block_list(void *base, std::uintptr_t list_offset);
  void init_lock_free_stack(ArenaHeader *header);
  SuperBlockChunk *init_super_block_chunk(void *base,
                                          std::uintptr_t chunk_offset);
  MediumChunk *init_medium_chunk(void *base, std::uintptr_t chunk_offset);
  void *get_arena_stats(ArenaHeader *header);
  void *get_super_block_pool(SuperBlockHeader *header);
  void *get_super_block_classes(SuperBlockHeader *header);
  void *get_super_block_list(void *base, std::uintptr_t list_offset);
  void *get_extent_manager_pool(MediumChunkHeader *header);
  void *get_lock_free_stack(ArenaHeader *header);
  void *get_chunk(void *base, std::uintptr_t chunk_offset);
  bool has_arena_space(ArenaStats *arena_stats,
                       std::uint64_t total_usable_size);
  bool has_super_block_space(ArenaStats *arena_stats,
                             std::uint64_t total_usable_size);
  SuperBlock *alloc_super_block(std::size_t id, SuperBlockHeader *header,
                                ExtentManager *extent_manager);
  SuperBlock *find_super_block(std::uintptr_t ptr, ArenaHeader *header);
  SuperBlock *get_super_block_class_or_null(std::size_t id,
                                            ArenaHeader *header);
  ArenaStats *get_arena_stats_pointer(ArenaHeader *header);
  LockFreeStack *get_lock_free_stack_pointer(ArenaHeader *header);
  void set_bytes_allocated(std::size_t size, bool is_extent,
                           ArenaHeader *header);
  void set_bytes_freed_extent(void *ptr, ArenaHeader *header);
  void set_bytes_freed_super_block(std::size_t size, ArenaHeader *header);
  void alloc_arena_header(std::uint32_t id, std::uint32_t core,
                          std::uint32_t flags_config);
  void alloc_super_block_chunk();
  void alloc_medium_chunk();
  void set_super_block_chunk_head(SuperBlockChunk *arena_chunk);
  void set_medium_chunk_head(MediumChunk *arena_chunk);
  void *init_memory(std::size_t size, std::size_t commit_size);
  void *find_medium_chunk(std::size_t size);
  void *super_block_allocation_path(SuperBlock *&super_block, std::size_t id,
                                    std::size_t size);

public:
  Arena(std::uint32_t id, std::uint32_t core, std::uint32_t flags_config);
  void *alloc(std::size_t id, std::size_t size);
  bool free(void *ptr);
  void free_remote(void *ptr);
  bool is_range_arena(std::uintptr_t ptr, ArenaHeader *header);
};
