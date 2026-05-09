#include "chunk.h"
#include "map_size.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <iterator>
#include <limits>
#include <optional>

class Tcache {
private:
  static constexpr uint16_t TCACHE_MAX_SIZE{512};
  static constexpr uint8_t NORMALIZED_SIZE{0};
  static constexpr uint8_t MAX_WASTE_ALLOWED{4};
  FreeNode *buckets[NUM_CLASSES];
  uint8_t counts[NUM_CLASSES];
  size_t min_effective_alignment(size_t requested_alignment) {
    if (requested_alignment == 0)
      return ALLOC_MIN_ALIGNMENT;
    size_t min_align{std::max(requested_alignment, ALLOC_MIN_ALIGNMENT)};
    return min_align;
  }
  bool within_waste(size_t class_size, size_t user_size) {
    double threshold_d{static_cast<double>(user_size) * MAX_WASTE_ALLOWED};
    // Potentailly a probblem should thhis be treted as an alignment or size
    // missmatch
    if (threshold_d > static_cast<double>(std::numeric_limits<size_t>::max()))
      return false;
    size_t threshold_s{static_cast<size_t>(threshold_d)};
    return class_size <= threshold_s;
  }
  SizeAlignmentResult map_size(std::size_t user_size,
                               size_t requested_alignment) {
    size_t new_alignment{min_effective_alignment(requested_alignment)};

    auto it{std::lower_bound(
        std::begin(map_info), std::end(map_info), user_size,
        [](const MapSizeAlignment &ms, size_t val) { return ms.size <= val; })};

    bool has_size{it != std::end(map_info)};
    if (!has_size)
      return {false, std::nullopt};

    for (; it != std::end(map_info); ++it) {
      size_t size{it->size};
      size_t alignment{it->alignment};
      if (alignment < requested_alignment)
        continue;
      if (!within_waste(size, user_size)) {
        return {true, std::nullopt};
      }
      size_t idx = std::distance(std::begin(map_info), it);
      return {true, static_cast<uint8_t>(idx)};
    }
  }

public:
  void *allocate_fast(std::size_t bytes, std::size_t requested_alignment) {
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
    SizeAlignmentResult map_result = map_size(bytes, requested_alignment);
    if (!map_result.has_size) {
      // go to the get_arena_memory_batch
    }
    if (map_result.has_size && !map_result.idx.has_value()) {
      // go to the slow path
    }
    uint8_t n{map_result.idx.value()};

    if (counts[n] == 0) {
      // ask for more block empty bin
      FreeNode *fill_bin{get_arena_memory_batch()};
      buckets[n] = fill_bin;
      ++counts[n];
    }

    FreeNode *head = buckets[n];
    --counts[n];
    head->next = nullptr;
    return reinterpret_cast<void *>(head);
    // 2. if so are there free chunks
    // 2a. if there are not but size do correspond to tcache sizes ask local
    // arena or global
    // 3. once we have free chhunks retrive the first one in the list
    // 4. diminish counts
    // 5. retrieve pointer
  }
  void free_fast(void *payload) {
    if (!payload)
      return;
    // use arena maybe compute arena id before calling arena
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
    void **trailer = reinterpret_cast<void **>(payload_addr - trailer_sz);

    *trailer = raw;
    header->payload_offset = static_cast<uint32_t>(payload_addr - base);

    return reinterpret_cast<void *>(payload_addr);
  }

  void free_aligned_trailer(void *payload) {
    if (!payload)
      return;
    std::uintptr_t p{reinterpret_cast<std::uintptr_t>(payload)};
    void **trailer{reinterpret_cast<void **>(p - sizeof(void *))};
    void *raw{*trailer};
    // free in this part with arena
  }

private:
  FreeNode *get_arena_memory_batch() {
    // arena will return a void* linked list size of list is to be determined
    void *arena_result;
    return static_cast<FreeNode *>(arena_result);
  }
  std::uintptr_t align_up(std::uintptr_t x, size_t a) {
    assert((a & (a - 1)) == 0);
    return (x + (a - 1)) & ~(uintptr_t)(a - 1);
  }
};
