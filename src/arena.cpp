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
    void *arena_base;
  };

  struct ArenaStats {
    std::atomic_uint_fast64_t alloc_count;
    std::atomic_uint_fast64_t free_count;
    std::atomic_uint_fast64_t bytes_allocated;
    std::atomic_uint_fast64_t bytes_free;
    std::uint8_t padding[(64 - ((4 * sizeof(uint64_t)) % 64)) % 64];
  };

  // SuperBlock nterface should go here
public:
  arena(std::uint32_t id, std::uint32_t core, std::uint32_t flags_config) {
    std::size_t arena_bytes{default_arena_size * 1024};
    std::size_t arena_bytes_with_header{arena_bytes};
    os_api::MemSpan mem_info;

    os_api::reserve_address_space(arena_bytes_with_header, os_api::PAGE_SIZE,
                                  mem_info);

    void *arena_base{mem_info.addr};

    ArenaHeader header{};
    header.version = 1;
    header.arena_id = id;
    header.owner_core = core;
    header.arena_base = arena_base;
  }
};
