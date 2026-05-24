#pragma once

#include "alloc_init.h"
#include "chunk.h"
#include <cstddef>

class TCache {
private:
  static constexpr std::size_t MIN_ALLOCATOR_SIZE{8};
  // I have to check if 100 makes sense
  static constexpr std::size_t MAX_BUCKET_ITEMS_THRESHOLD{100};
  static constexpr int NOCLASS{-1};
  FreeNode *buckets[NUM_CLASSES]{};
  int count[NUM_CLASSES]{};
  AllocRoute *alloc_route{nullptr};
  int find(std::size_t size);

public:
  TCache();
  void *allocate(std::size_t size);
  void free(void *raw);
};
