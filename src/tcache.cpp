#include "tcache.h"
#include <algorithm>
#include <iterator>

TCache::TCache() : alloc_route(get_alloc_route()){};

int TCache::find(std::size_t size) {
  auto it{std::lower_bound(std::begin(map_info), std::end(map_info), size,
                           [](MapSizeAlignment &map_item, std::size_t sz) {
                             return map_item.size < sz;
                           })};

  return std::distance(std::begin(map_info), it);
};

void *TCache::allocate(std::size_t size) {
  if (size < MIN_ALLOCATOR_SIZE)
    size = MIN_ALLOCATOR_SIZE;

  int idx{find(size)};

  if (buckets[idx]) {
    FreeNode *buket_list{buckets[idx]};
    count[idx]--;
    FreeNode *res = buket_list;
    buket_list = buket_list->next;
    res->next = nullptr;
    return reinterpret_cast<void *>(res);
  } else {
  }
};
