#pragma once

#include "alloc_route.h"
#include "arena.h"
#include "chunk.h"
#include <cstddef>
#include <utility>

class Tcache {
private:
  FreeNode *bucket[NUM_CLASSES];
  int count[NUM_CLASSES];
  AllocRoute *alloc_route{nullptr};
  std::pair<std::size_t, std::size_t> find(std::size_t size);

public:
  Tcache();
  void *allocate(std::size_t size);
  void free(void *raw);
};
