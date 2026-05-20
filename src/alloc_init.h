#pragma once

#include "alloc_route.h"
#include "arena.h"
#include <new>
#include <utility>

#ifdef __linux__
#include <sched.h>
#include <unistd.h>
inline int get_num_cores() { return sysconf(_SC_NPROCESSORS_ONLN); }
inline int get_current_core() { return sched_getcpu(); }
#elif _WIN32
#include <windows.h>
inline int get_num_cores() {
  SYSTEM_INFO info;
  GetSystemInfo(&info);
  return info.dwNumberOfProcessors;
}
inline int get_current_core() { return GetCurrentProcessorNumber; }
#else
#error "Unsupported platform"
#endif

inline Arena **arena_pool{nullptr};

struct InitState {
  Arena **arenas;
  AllocRoute *route;
  int num_cores;
};

inline InitState global_init_state{};

inline AllocRoute *init_alloc_route() {
  os_api::MemSpan route_mem;
  os_api::OsResult res{os_api::reserve_address_space(
      sizeof(AllocRoute), os_api::PAGE_SIZE, route_mem)};
  if (res != os_api::OsResult::Success) {
    throw std::bad_alloc{};
  }

  os_api::commit_memory(route_mem.addr, route_mem.size);
  AllocRoute *route{reinterpret_cast<AllocRoute *>(route_mem.addr)};
  return route;
};

inline std::pair<int, Arena **> init_arena_pool(std::size_t alignment,
                                                std::uint32_t flags) {
  // Here we can tweak a bit the configuration either by passing flags or having
  // a didecated behavior per arena we could even create more arenas if needed
  // and reserve them alignmen should be dictated by user for now 0
  int num_cores{get_num_cores()};
  os_api::MemSpan pool_mem;
  os_api::OsResult pool_res{os_api::reserve_address_space(
      sizeof(Arena *) * num_cores, os_api::PAGE_SIZE, pool_mem)};
  if (pool_res != os_api::OsResult::Success) {
    throw std::bad_alloc{};
  }
  os_api::commit_memory(pool_mem.addr, pool_mem.size);
  arena_pool = reinterpret_cast<Arena **>(pool_mem.addr);

  for (std::uint32_t i{}; i < num_cores; i++) {
    os_api::MemSpan mem;
    os_api::OsResult res =
        os_api::reserve_address_space(sizeof(Arena), alignment, mem);
    if (res != os_api::OsResult::Success) {
      throw std::bad_alloc{};
    }
    os_api::commit_memory(mem.addr, mem.size);

    Arena *arena = new (mem.addr) Arena{i, i, flags};

    arena_pool[i] = arena;
  }

  return {num_cores, arena_pool};
};

inline void init_allocator(std::size_t alignment, std::uint32_t flags) {

  AllocRoute *alloc_route{init_alloc_route()};
  std::pair<int, Arena **> pool_res{init_arena_pool(alignment, flags)};

  global_init_state.route = alloc_route;
  global_init_state.arenas = pool_res.second;
  global_init_state.num_cores = pool_res.first;
};

inline AllocRoute *get_alloc_route() { return global_init_state.route; };

inline Arena **get_arena_pool() { return global_init_state.arenas; };
