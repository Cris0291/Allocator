#include "alloc_route.h"

AllocRoute::AllocRoute(Arena **arenas, int cores)
    : num_cores(cores), arena_pool(arenas){};

void *AllocRoute::alloc_thread_route(std::size_t id, std::size_t size) {
  // perhaps in the future here we can hanlde more things
  // like no more meore form the 256k block so alloc a new one we will see
  // void* raw couls be nullptr for now just impleemnt the simple case where it
  // is not
  Arena *arena{arena_pool[sched_getcpu()]};
  void *raw{arena->alloc(id, size)};
  return raw;
};

void AllocRoute::free_thread_route(void *ptr) {
  Arena *arena{nullptr};

  arena = arena_pool[sched_getcpu()];
  bool is_local = arena->free(ptr);

  if (is_local)
    return;

  std::uintptr_t ptr_addr{reinterpret_cast<std::uintptr_t>(ptr)};
  for (int i{}; i < num_cores; i++) {
    arena = arena_pool[i];
    bool is_range = arena->is_range_arena(ptr_addr);
    if (is_range) {
      arena->free_remote(ptr);
      return;
    }
  }
  // Huge memory free should be here
};
