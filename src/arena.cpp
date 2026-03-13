#include "os_api.h"
#include <atomic>
#include <cstddef>
#include <cstdint>

class arena {
  std::size_t default_arena_size{256};
  struct ArenaHeader {
    std::uint32_t version;
    std::uint32_t magic;
    std::uint32_t flags;
    std::uint16_t arena_id;
    std::uint16_t owner_core;
    std::uint64_t total_size;
    std::uint32_t usable_offset;
    std::uint32_t usable_size;
    std::uint32_t extend_root_off;
    std::uint32_t per_class_off;
    std::uint8_t per_class_count;
    std::uint8_t reserved1[3];
    std::uint32_t remote_queue_off;
    std::uint32_t stats_off;
    std::uint64_t header_checksum;
    std::uint8_t reserved[64];
  };

  struct PerClassMeta {
    std::uint32_t block_size;
    std::uint32_t freelist_head_off;
    std::uint32_t page_owned;
    std::uint32_t free_count;
  };

  struct ArenaStats {
    std::atomic_uint_fast64_t alloc_count;
    std::atomic_uint_fast64_t free_count;
    std::atomic_uint_fast64_t bytes_allocated;
    std::atomic_uint_fast64_t bytes_free;
    std::uint8_t padding[(64 - ((4 * sizeof(uint64_t)) % 64)) % 64];
  };

public:
  arena(std::uint32_t id, std::uint32_t core, std::uint32_t flags_config) {
    std::size_t arena_bytes{default_arena_size * 1024};
    std::size_t arena_bytes_with_header{arena_bytes + (2 * os_api::PAGE_SIZE)};
    os_api::MemSpan mem_info;

    os_api::reserve_address_space(arena_bytes_with_header, os_api::PAGE_SIZE,
                                  mem_info);

    void *arena_base{mem_info.addr};
    std::uintptr_t base{reinterpret_cast<std::uintptr_t>(arena_base)};
    ArenaHeader *arena_header{reinterpret_cast<ArenaHeader *>(arena_base)};
    // goiing to use an initializer/orchestrator in order to create an arena per
    // core
    arena_header->total_size = arena_bytes;
    arena_header->arena_id = id;
    arena_header->owner_core = core;
    arena_header->flags = flags_config;
    arena_header->total_size = arena_bytes;
    arena_header->usable_offset = os_api::PAGE_SIZE;
    arena_header->usable_size = default_arena_size;
    // offset to hot/dynamiic metadata could be related to the five classes for
    // now a place holder
    arena_header->extend_root_off = os_api::PAGE_SIZE;
    arena_header->per_class_count = 5;
    // point to a table of per class metadata
    arena_header->per_class_off = 4096;
    arena_header->per_class_count = 5;
    // i have to calculate some offset yet this is aplace hollder
    arena_header->remote_queue_off = 11;
    arena_header->stats_off = 11;
    arena_header->header_checksum = 1;

    for (int i{}; arena_header->per_class_count; i++) {
      auto entry =
          base + arena_header->per_class_off + (i * sizeof(PerClassMeta));
    }
  }
};
