#include "chunk.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <ctime>

class Tcache {
private:
  static constexpr uint8_t MAX_CLASSES{18};
  static constexpr uint16_t TCACHE_MAX_SIZE{512};
  static constexpr uint8_t NORMALIZED_SIZE{0};
  FreeNode *buckets[MAX_CLASSES];
  uint8_t counts[MAX_CLASSES];
  uint8_t map_size(std::size_t size) {
    if (size <= 4)
      return 0;
    if (size <= 8)
      return 1;
    if (size <= 16)
      return 2;
    if (size <= 24)
      return 3;
    if (size <= 32)
      return 4;
    if (size <= 40)
      return 5;
    if (size <= 48)
      return 6;
    if (size <= 64)
      return 7;
    if (size <= 80)
      return 8;
    if (size <= 96)
      return 9;
    if (size <= 128)
      return 10;
    if (size <= 160)
      return 11;
    if (size <= 192)
      return 12;
    if (size <= 256)
      return 13;
    if (size <= 320)
      return 14;
    if (size <= 384)
      return 15;
    if (size <= 448)
      return 16;
    if (size <= 512)
      return 17;

    return 18;
  }

public:
  void *allocate(std::size_t bytes) {
    // normalized 0 bytes size
    if (bytes == 0) {
      // cache hit
      if (counts[NORMALIZED_SIZE] != 0) {
        FreeNode *head = buckets[NORMALIZED_SIZE];
        FreeNode *next = head->next;
        buckets[NORMALIZED_SIZE] = next;
        --counts[NORMALIZED_SIZE];
        head->next = nullptr;
        return static_cast<void *>(head);
      } else {
        // go to thhe arena for more freeNodes
      }
    }
    uint8_t n = map_size(bytes);
    // 2. if so are there free chunks
    // 2a. if there are not but size do correspond to tcache sizes ask local
    // arena or global
    // 3. once we have free chhunks retrive the first one in the list
    // 4. diminish counts
    // 5. retrieve pointer
  }
  void *allocate_aligned_trailer(size_t bytes, size_t requested_align) {
    if (requested_align == 0)
      requested_align = alignof(std::max_align_t);
    std::size_t effective_alignment =
        std::max<size_t>(requested_align, alignof(void *));
    assert((effective_alignment & (effective_alignment - 1)) == 0);

    std::size_t header_sz{sizeof(ChunkHeader)};
    std::size_t trailer_sz{sizeof(void *)};
    std::size_t padding{effective_alignment - 1};

    std::size_t chunk_size{header_sz + bytes + trailer_sz + padding};

    void *raw; // use arena to retrieve the value;

    std::uintptr_t base{reinterpret_cast<uintptr_t>(raw)};
    ChunkHeader *header{reinterpret_cast<ChunkHeader *>(base)};
    header->chunk_size = static_cast<uint32_t>(chunk_size);
    header->flags = 0;

    std::uintptr_t payload_addr =
        align_up(base + header_sz + trailer_sz, effective_alignment);
    void **trailer = reinterpret_cast<void **>(payload_addr - base);

    *trailer = raw;
    header->payload_offset = static_cast<uint32_t>(payload_addr - base);

    return reinterpret_cast<void *>(payload_addr);
  }

private:
  void *get_arena_memory_batch() {}
  std::uintptr_t align_up(std::uintptr_t x, size_t a) {
    assert((a & (a - 1)) == 0);
    return (x + (a - 1)) & ~(uintptr_t)(a - 1);
  }
};
