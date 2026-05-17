#pragma once

#include "arena.h"
#include <cstddef>
#include <cstdint>
#include <sched.h>
#include <thread>
// for now we use this varibales numer of cores or
// sched_getcpu but perhaps in teh future i will abstract this
// so that i can make this cross platform

class AllocRoute {
private:
  unsigned int num_cores{};
  Arena **arena_pool;

public:
  AllocRoute(Arena **arenas, int cores);
  void *alloc_thread_route(std::size_t id, std::size_t size);
  void free_thread_route(void *ptr);
};
