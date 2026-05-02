#include "extent_manager.h"
#include "os_api.h"
#include "super_block.h"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <uchar.h>

class arena {
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
    SuperBlock *super_block_pool_classes[NUM_CLASSES];
  };

  ExtentManager *extent_manager;

  struct ExtentManagerPool {
    ExtentManager::Entry entry_pool[64];
  };

  ArenaHeader *base;

  inline static std::uintptr_t align_up(std::uintptr_t x, std::size_t size) {
    return (x + (size - 1)) & ~(size - 1);
  };

  static ArenaHeader *init_header(void *base, std::uint32_t id,
                                  std::uint32_t core,
                                  std::uint32_t flags_config) {
    ArenaHeader *arena_base{reinterpret_cast<ArenaHeader *>(base)};
    arena_base->arena_id = id;
    arena_base->owner_core = core;
    arena_base->arena_base = base;
    arena_base->flags = flags_config;
    arena_base->version = 1;
    arena_base->magic = 0xA17ECAFE;

    std::uintptr_t base_ptr{reinterpret_cast<std::uintptr_t>(base)};
    std::uintptr_t header_offset{sizeof(ArenaHeader)};
    std::uintptr_t hreader_alignup_offset{align_up(header_offset, 64)};

    // Offset   Size   Description
    //-----    ----   -----------
    // 0         88    ArenaHeader(64 bit)
    // 88        40    Padding for the next cache line
    // 128       64    ArenaStats
    // 192       64    SuperBlockHealth
    // 256       192   SuperBlockPool
    // 448       136   SuperBlockClasses
    // 592       40    ExtentManager
    // 632      3072   Extent manager pool
    // 3704     392    Padding to usable memory
    // 4096
    arena_base->arena_stats_offset = hreader_alignup_offset;
    arena_base->super_block_health_offset =
        arena_base->arena_stats_offset + sizeof(ArenaStats);
    arena_base->super_block_pool_offset =
        arena_base->super_block_health_offset + sizeof(SuperBlockHealth);
    arena_base->super_block_classes_offset =
        arena_base->super_block_pool_offset + sizeof(SuperBlockPool);
    arena_base->extent_manager_offset =
        arena_base->super_block_classes_offset + sizeof(SuperBlockPoolClasses);
    arena_base->extent_manager_pool_offset =
        arena_base->extent_manager_offset + sizeof(ExtentManager);

    arena_base->usable_region_offset =
        align_up(arena_base->extent_manager_pool_offset, 4096);

    arena_base->total_usable_size =
        (default_arena_size - arena_base->usable_region_offset) * 1024;

    return arena_base;
  };

  static void init_arena_stats(ArenaHeader *header) {
    ArenaStats *arena_stats{reinterpret_cast<ArenaStats *>(
        reinterpret_cast<std::uintptr_t>(header->arena_base) +
        header->arena_stats_offset)};
    // This wiil only be handled by the owning thread of the core
    arena_stats->alloc_count.store(0, std::memory_order_release);
    arena_stats->free_count.store(0, std::memory_order_release);
    arena_stats->bytes_free.store(256 * 1024, std::memory_order_release);
    arena_stats->bytes_allocated.store(0, std::memory_order_release);
    arena_stats->bytes_in_use.store(0, std::memory_order_release);
    arena_stats->bytes_wasted.store(0, std::memory_order_release);
  };

  void init_extent_manager(ArenaHeader *header) {
    void *extent_manager_addr = reinterpret_cast<void *>(
        reinterpret_cast<std::uintptr_t>(header->arena_base) +
        header->extent_manager_offset);

    std::uintptr_t arena_base{
        reinterpret_cast<std::uintptr_t>(header->arena_base)};

    std::uintptr_t pool_base{arena_base + header->extent_manager_pool_offset};
    std::size_t pool_size{sizeof(ExtentManagerPool)};
    std::uintptr_t usable_base{arena_base + header->usable_region_offset};
    std::size_t usable_size{header->total_usable_size};

    extent_manager = new (extent_manager_addr)
        ExtentManager(pool_base, pool_size, usable_base, usable_size);
  };

  void *get_super_pool_address() {
    return reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(base) +
                                    base->super_block_pool_offset);
  };

  void *get_super_block_health_address() {
    return reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(base) +
                                    base->super_block_health_offset);
  };

  bool alloc_super_block_node(std::size_t slot_size) {
    void *node_pool_addr{get_super_pool_address()};
    void *super_block_health_addr{get_super_block_health_address()};
    // Have doubst in here
    // flow should be load super_block_health_addr->superblocks_active
    // then check if at that moment they are less than 4 if so proceed since we
    // can only have 4 of then i wonder if super bclok pool should be atomic in
    // iorder to see if a super block for that size is alrady here then recheck
    // for 4 and for empty slot and if so allocate
  };

public:
  arena(std::uint32_t id, std::uint32_t core, std::uint32_t flags_config) {
    std::size_t arena_bytes{default_arena_size * 1024};
    os_api::MemSpan mem_info;

    os_api::reserve_address_space(arena_bytes, os_api::PAGE_SIZE, mem_info);

    void *base{mem_info.addr};

    ArenaHeader *arena_base = init_header(base, id, core, flags_config);
    base = arena_base->arena_base;
    init_arena_stats(arena_base);
    init_extent_manager(arena_base);

    os_api::commit_memory(base, 4096);
  };
  void *alloc(std::size_t size) {

  };
};
