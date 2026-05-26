#include "tcache.h"
#include "arena.h"
#include <algorithm>
#include <iterator>

TCache::TCache() : alloc_route(get_alloc_route()){};

TCache::~TCache() {
  FreeNode *node{nullptr};
  for (int i{}; i < NUM_CLASSES; i++) {
    while (buckets[i]) {
      node = buckets[i];
      buckets[i] = node->next;
      node->next = nullptr;
      alloc_route->free_thread_route(reinterpret_cast<void *>(node));
      count[i]--;
    }
  }
};

int TCache::find(std::size_t size) {
  auto it{std::lower_bound(std::begin(map_info), std::end(map_info), size,
                           [](MapSizeAlignment &map_item, std::size_t sz) {
                             return map_item.size < sz;
                           })};

  if (it == std::end(map_info))
    return NOCLASS;
  return std::distance(std::begin(map_info), it);
};

void TCache::flush_bucket(std::size_t id) {
  FreeNode *node{nullptr};
  int bucket_to_flush{count[id] / 2};
  for (int i{}; i < bucket_to_flush; i++) {
    node = buckets[id];
    buckets[id] = node->next;
    node->next = nullptr;
    count[id]--;
    alloc_route->free_thread_route(reinterpret_cast<void *>(node));
  }
};

void *TCache::allocate(std::size_t size) {
  if (size < MIN_ALLOCATOR_SIZE)
    size = MIN_ALLOCATOR_SIZE;

  int idx{find(size)};

  if (idx == NOCLASS) {
    void *raw_extent{alloc_route->alloc_thread_route(NOCLASS, size)};
    return raw_extent;
  }

  if (buckets[idx] && count[idx] != 0) {
    FreeNode *buket_list{buckets[idx]};
    buckets[idx] = buket_list->next;
    count[idx]--;
    buket_list->next = nullptr;
    return reinterpret_cast<void *>(buket_list);
  } else {
    // also here we must refill not one but many chunks at once
    void *raw{alloc_route->alloc_thread_route(idx, map_info[idx].size)};
    return raw;
  }
};

void TCache::free(void *raw) {
  std::uintptr_t ptr{reinterpret_cast<std::uintptr_t>(raw)};
  std::uintptr_t super_block_base{ptr & ~(SuperBlock::span_size - 1)};
  SuperBlock::SuperBlockHeader *header{
      reinterpret_cast<SuperBlock::SuperBlockHeader *>(super_block_base)};

  if (header->super_block_magic == SuperBlock::SUPER_BLOCK_MAGIC) {
    FreeNode *node{reinterpret_cast<FreeNode *>(raw)};
    node->next = buckets[header->class_id];
    buckets[header->class_id] = node;
    count[header->class_id]++;
    if (count[header->class_id] >= MAX_BUCKET_ITEMS_THRESHOLD) {
      flush_bucket(header->class_id);
    }
  } else {
    alloc_route->free_thread_route(raw);
  }
};
