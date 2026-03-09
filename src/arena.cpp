#include "os_api.h"
#include <cstddef>
#include <cstdint>

class arena {
  int default_arena_size;
  struct arena_header {
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
  }

  public : arena(int arena_size = 256)
      : default_arena_size{arena_size} {
    if (default_arena_size <= 0 || default_arena_size > 256)
      default_arena_size = 256;
    std::size_t arena_bytes{256 * 1024};
    os_api::MemSpan mem_info;

    os_api::reserve_address_space(arena_bytes + os_api::PAGE_SIZE,
                                  os_api::PAGE_SIZE, mem_info);
  }
};
