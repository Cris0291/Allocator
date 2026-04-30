#include "os_api.h"
#include "super_block.h"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <uchar.h>

class arena {
private:
  static constexpr std::size_t NUM_CLASSES{17};
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
    std::uintptr_t node_pool_offset;
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
    SuperBlock *super_block_pool[NUM_CLASSES];
  };

  inline static std::uintptr_t align_up(std::uintptr_t x, std::size_t size) {
    return (x + (size - 1)) & ~(size - 1);
  };

  static void init_header(void *base, std::uint32_t id, std::uint32_t core,
                          std::uint32_t flags_config) {
    ArenaHeader *arena_base{reinterpret_cast<ArenaHeader *>(base)};
    arena_base->arena_id = id;
    arena_base->owner_core = core;
    arena_base->arena_base = base;
    arena_base->flags = flags_config;
    arena_base->total_usable_size = 256;
    arena_base->version = 1;
    arena_base->magic = 0xA17ECAFE;

    std::uintptr_t base_ptr{reinterpret_cast<std::uintptr_t>(base)};
    std::uintptr_t header_offset{base_ptr + sizeof(ArenaHeader)};
    std::uintptr_t hreader_alignup_offset{align_up(header_offset, 64)};

    // Offset   Size   Description
    //-----    ----   -----------
    // 0         56    ArenaHeader
    // 56        8     padding to 64
    // 64        64    ArenaStats
    // 128       64    SuperBlockHealth
    // 192       136   SuperBlockPool
    // 328       56    if i want another cache inline
    // 384
    arena_base->arena_stats_offset = hreader_alignup_offset;
    arena_base->super_block_health_offset =
        arena_base->arena_stats_offset + sizeof(ArenaStats);
    arena_base->node_pool_offset =
        arena_base->super_block_health_offset + sizeof(SuperBlockHealth);
  };

public:
  arena(std::uint32_t id, std::uint32_t core, std::uint32_t flags_config) {
    std::size_t arena_bytes{default_arena_size * 1024};
    os_api::MemSpan mem_info;

    os_api::reserve_address_space(arena_bytes, os_api::PAGE_SIZE, mem_info);

    void *arena_base{mem_info.addr};
    init_header(arena_base, id, core, flags_config);
  }
};
