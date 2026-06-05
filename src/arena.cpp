#include "arena.h"
#include "extent_manager.h"
#include "super_block.h"
#include <cstddef>
#include <cstdint>

std::uintptr_t Arena::align_up(std::uintptr_t x, std::size_t size) {
  return (x + (size - 1)) & ~(size - 1);
};

Arena::ArenaHeader *Arena::init_arena_header(void *base, std::uint32_t id,
                                             std::uint32_t core,
                                             std::uint32_t flags_config) {
  ArenaHeader *arena_base{reinterpret_cast<ArenaHeader *>(base)};
  arena_base->arena_id = id;
  arena_base->owner_core = core;
  arena_base->flags = flags_config;
  arena_base->version = 1;
  arena_base->magic = 0xA17ECAFE;
  arena_base->super_block_count = 0;
  arena_base->middle_chunk_count = 0;
  arena_base->total_usable_size = 0;
  arena_base->arena_base = base;

  std::uintptr_t base_ptr{reinterpret_cast<std::uintptr_t>(base)};
  std::uintptr_t header_offset{sizeof(ArenaHeader)};
  std::uintptr_t header_alignup_offset{align_up(header_offset, 64)};

  // Offset   Size   Description
  //-----    ----   -----------
  // 0         104    ArenaHeader(64 bit)
  // 104       24    Padding for the next cache line
  // 128       64    ArenaStats
  // 192       64    SuperBlockHealth
  // 256       192   SuperBlockPool
  // 448       136   SuperBlockClasses
  // 592       40    ExtentManager
  // 632      3072   Extent manager pool
  // 3704      8     Lock free stack
  // 3710     24     ArenaChunk
  // 3734     362    Padding to usable memory
  // 4096
  arena_base->arena_stats_offset = header_alignup_offset;
  arena_base->super_block_active_offset =
      arena_base->arena_stats_offset + sizeof(ArenaStats);
  arena_base->super_block_partial_offset =
      arena_base->super_block_active_offset + sizeof(SuperBlockActiveList);
  arena_base->super_block_full_offset =
      arena_base->super_block_partial_offset + sizeof(SuperBlockFullList);

  arena_base->lock_free_stack_offset =
      arena_base->super_block_full_offset + sizeof(SuperBlockFullList);

  return arena_base;
};

Arena::SuperBlockHeader *
Arena::init_super_block_header(void *super_block_chunk_base, std::uint32_t id) {
  SuperBlockHeader *super_block_header{
      reinterpret_cast<SuperBlockHeader *>(super_block_chunk_base)};
  super_block_header->super_block_header_id = id;
  super_block_header->total_used_size = 0;

  super_block_header->super_block_pool_offset = sizeof(SuperBlockHeader);
  super_block_header->super_block_classes_offset =
      super_block_header->super_block_pool_offset + sizeof(SuperBlockPool);
  super_block_header->extent_manager_offset =
      super_block_header->super_block_classes_offset +
      sizeof(SuperBlockPoolClasses);
  super_block_header->extent_manager_pool_offset =
      super_block_header->extent_manager_offset + sizeof(ExtentManager);
  super_block_header->super_block_chunk_offset =
      super_block_header->extent_manager_pool_offset +
      sizeof(ExtentManagerPool);

  super_block_header->usable_region_offset =
      align_up(super_block_header->super_block_chunk_offset, 4096);

  super_block_header->total_usable_size =
      (default_arena_size * 1024) - super_block_header->usable_region_offset;

  return super_block_header;
};

Arena::MediumChunkHeader *Arena::init_medium_header(void *medium_chunk_base,
                                                    std::uint32_t id) {
  MediumChunkHeader *medium_chunk_header{
      reinterpret_cast<MediumChunkHeader *>(medium_chunk_base)};
  medium_chunk_header->medium_chunk_header_id = id;
  medium_chunk_header->total_used_size = 0;

  medium_chunk_header->extent_manager_offset = sizeof(MediumChunkHeader);
  medium_chunk_header->extent_manager_pool_offset =
      medium_chunk_header->extent_manager_offset + sizeof(ExtentManager);
  medium_chunk_header->medium_chunk_offset =
      medium_chunk_header->extent_manager_pool_offset +
      sizeof(ExtentManagerPool);

  medium_chunk_header->usable_region_offset =
      align_up(medium_chunk_header->medium_chunk_offset, 4096);

  medium_chunk_header->total_usable_size =
      (default_arena_size * 1024) - medium_chunk_header->usable_region_offset;

  return medium_chunk_header;
};

void Arena::init_arena_stats(ArenaHeader *header) {
  ArenaStats *arena_stats{reinterpret_cast<ArenaStats *>(
      reinterpret_cast<std::uintptr_t>(header->arena_base) +
      header->arena_stats_offset)};
  // This wiil only be handled by the owning thread of the core
  arena_stats->alloc_count.store(0, std::memory_order_release);
  arena_stats->free_count.store(0, std::memory_order_release);
  arena_stats->bytes_free.store(256 * 1024, std::memory_order_release);
  arena_stats->bytes_allocated.store(0, std::memory_order_release);
  arena_stats->bytes_in_use.store(0, std::memory_order_release);
  arena_stats->bytes_wasted.store(0, std::memory_order_release);
};

ExtentManager *
Arena::init_extent_manager(void *base, std::uintptr_t extent_manager_offset,
                           std::uintptr_t extent_manager_pool_offset,
                           std::uintptr_t usable_region_offset,
                           std::uint64_t total_usable_size) {
  void *extent_manager_addr = reinterpret_cast<void *>(
      reinterpret_cast<std::uintptr_t>(base) + extent_manager_offset);

  std::uintptr_t arena_base{reinterpret_cast<std::uintptr_t>(base)};

  std::uintptr_t pool_base{arena_base + extent_manager_pool_offset};
  std::size_t pool_size{sizeof(ExtentManagerPool)};
  std::uintptr_t usable_base{arena_base + usable_region_offset};
  std::size_t usable_size{total_usable_size};

  ExtentManager *extent_manager{new (extent_manager_addr) ExtentManager(
      pool_base, pool_size, usable_base, usable_size)};
  return extent_manager;
};

void Arena::init_super_block_pool(SuperBlockHeader *header) {
  void *super_block_pool_addr{get_super_block_pool(header)};
  new (super_block_pool_addr) SuperBlockPool();
};

void Arena::init_super_block_classes(SuperBlockHeader *header) {
  void *super_block_classes_addr{get_super_block_classes(header)};
  new (super_block_classes_addr) SuperBlockPoolClasses();
};

void Arena::init_lock_free_stack(ArenaHeader *header) {
  void *lock_free_stack_addr{get_lock_free_stack(header)};
  new (lock_free_stack_addr) LockFreeStack();
};

Arena::SuperBlockChunk *
Arena::init_super_block_chunk(void *base, std::uintptr_t chunk_offset) {
  void *arena_chunk_addr{get_chunk(base, chunk_offset)};
  SuperBlockChunk *arena_chunk{new (arena_chunk_addr) SuperBlockChunk()};
  return arena_chunk;
};

Arena::MediumChunk *Arena::init_medium_chunk(void *base,
                                             std::uintptr_t chunk_offset) {
  void *arena_chunk_addr{get_chunk(base, chunk_offset)};
  MediumChunk *arena_chunk{new (arena_chunk_addr) MediumChunk()};
  return arena_chunk;
};

Arena::SuperBlockList *
Arena::init_super_block_list(void *base, std::uintptr_t list_offset) {
  void *super_block_list_addr{get_super_block_list(base, list_offset)};
  SuperBlockList *super_block_list{new (super_block_list_addr)
                                       SuperBlockList()};
};

void *Arena::get_super_block_list(void *base, std::uintptr_t list_offset) {
  return reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(base) +
                                  list_offset);
};

void *Arena::get_arena_stats(ArenaHeader *header) {
  return reinterpret_cast<void *>(
      reinterpret_cast<std::uintptr_t>(header->arena_base) +
      header->arena_stats_offset);
};

void *Arena::get_super_block_pool(SuperBlockHeader *header) {
  return reinterpret_cast<void *>(
      reinterpret_cast<std::uintptr_t>(header->base) +
      header->super_block_pool_offset);
};

void *Arena::get_super_block_classes(SuperBlockHeader *header) {
  return reinterpret_cast<void *>(
      reinterpret_cast<std::uintptr_t>(header->base) +
      header->super_block_classes_offset);
};

void *Arena::get_extent_manager_pool(MediumChunkHeader *header) {
  return reinterpret_cast<void *>(
      reinterpret_cast<std::uintptr_t>(header->base) +
      header->extent_manager_pool_offset);
};

void *Arena::get_lock_free_stack(ArenaHeader *header) {
  return reinterpret_cast<void *>(
      reinterpret_cast<std::uintptr_t>(header->arena_base) +
      header->lock_free_stack_offset);
};

void *Arena::get_chunk(void *base, std::uintptr_t chunk_offset) {
  return reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(base) +
                                  chunk_offset);
};

bool Arena::has_arena_space(ArenaStats *arena_stats,
                            std::uint64_t total_usable_size) {
  return total_usable_size >
         arena_stats->bytes_allocated.load(std::memory_order_acquire);
};

bool Arena::has_super_block_space(ArenaStats *arena_stats,
                                  std::uint64_t total_usable_size) {
  return total_usable_size -
             arena_stats->bytes_allocated.load(std::memory_order_acquire) >=
         SuperBlock::get_super_block_size();
}

SuperBlock *Arena::alloc_super_block(std::size_t id, ArenaHeader *header,
                                     ExtentManager *extent_manager) {
  // Have doubst in here
  // flow should be load super_block_health_addr->superblocks_active
  // then check if at that moment they are less than 4 if so proceed since we
  // can only have 4 of then i wonder if super bclok pool should be atomic in
  // iorder to see if a super block for that size is alrady here then recheck
  // for 4 and for empty slot and if so allocate
  SuperBlock *super_block{nullptr};
  void *super_block_pool_addr{get_super_block_pool(header)};

  SuperBlockPool *super_block_pool{
      reinterpret_cast<SuperBlockPool *>(super_block_pool_addr)};

  void *super_block_pool_classes_offset{get_super_block_classes(header)};

  SuperBlockPoolClasses *super_block_pool_classes{
      reinterpret_cast<SuperBlockPoolClasses *>(
          super_block_pool_classes_offset)};

  for (int i{}; i < 4; i++) {
    if (super_block_pool->occupied[i])
      continue;

    void *super_block_addr{
        reinterpret_cast<void *>(super_block_pool->storage[i])};
    super_block = new (super_block_addr)
        SuperBlock(id, *extent_manager, map_info[id].size);
    super_block_pool_classes->super_block_pool_classes[id] = super_block;
    super_block_pool->occupied[i] = true;
    ArenaStats *stats{reinterpret_cast<ArenaStats *>(get_arena_stats(header))};
    stats->bytes_allocated.fetch_add(SuperBlock::get_super_block_size(),
                                     std::memory_order_release);
    break;
  }

  return super_block;
};

SuperBlock *Arena::find_super_block(std::uintptr_t ptr, ArenaHeader *header) {
  // potential refactor with previous function
  // maybe add a template to accept a lanbda function the idea is
  // to be able to dins a super block on different consitions we will see
  SuperBlock *super_block{nullptr};
  void *super_block_pool_addr{get_super_block_pool(header)};

  SuperBlockPool *super_block_pool{
      reinterpret_cast<SuperBlockPool *>(super_block_pool_addr)};

  for (int i{}; i < 4; i++) {
    if (!super_block_pool->occupied[i])
      continue;

    void *super_block_addr{
        reinterpret_cast<void *>(super_block_pool->storage[i])};
    super_block = reinterpret_cast<SuperBlock *>(super_block_addr);
    if (super_block->is_range(ptr))
      return super_block;
  }

  return nullptr;
};

SuperBlock *Arena::get_super_block_class_or_null(std::size_t id,
                                                 ArenaHeader *header) {
  void *super_block_classes{get_super_block_classes(header)};
  SuperBlockPoolClasses *pool_classes{
      reinterpret_cast<SuperBlockPoolClasses *>(super_block_classes)};
  SuperBlock **classes_array{pool_classes->super_block_pool_classes};
  if (classes_array[id])
    return classes_array[id];
  return nullptr;
}

Arena::ArenaStats *Arena::get_arena_stats_pointer(ArenaHeader *header) {
  void *arena_stats_addr{get_arena_stats(header)};
  ArenaStats *arena_stats{reinterpret_cast<ArenaStats *>(arena_stats_addr)};
  return arena_stats;
}

LockFreeStack *Arena::get_lock_free_stack_pointer(ArenaHeader *header) {
  void *lock_free_stack_addr{get_lock_free_stack(header)};
  return reinterpret_cast<LockFreeStack *>(lock_free_stack_addr);
};

void Arena::set_bytes_allocated(std::size_t size, bool is_extent,
                                ArenaHeader *header) {
  std::size_t final_size{is_extent ? size + ExtentManager::HEADER_SIZE : size};
  ArenaStats *arena_stats{get_arena_stats_pointer(header)};
  arena_stats->bytes_allocated.fetch_add(final_size, std::memory_order_release);
}

void Arena::set_bytes_freed_extent(void *ptr, ArenaHeader *header) {
  ArenaStats *arena_stats{get_arena_stats_pointer(header)};
  std::size_t size{ExtentManager::get_header_size(ptr)};
  arena_stats->bytes_free.fetch_add(size + ExtentManager::HEADER_SIZE,
                                    std::memory_order_release);
}

void Arena::set_bytes_freed_super_block(std::size_t size, ArenaHeader *header) {
  ArenaStats *arena_stats{get_arena_stats_pointer(header)};
  arena_stats->bytes_free.fetch_add(size, std::memory_order_release);
}

void *Arena::init_memory(std::size_t size, std::size_t commit_size) {
  std::size_t arena_bytes{size};
  os_api::MemSpan mem_info;
  os_api::reserve_address_space(arena_bytes, os_api::PAGE_SIZE, mem_info);
  void *base{mem_info.addr};
  os_api::commit_memory(base, commit_size);
};

void Arena::alloc_arena_header(std::uint32_t id, std::uint32_t core,
                               std::uint32_t flags_config) {
  void *base{init_memory(os_api::PAGE_SIZE, os_api::PAGE_SIZE)};

  ArenaHeader *arena_base = init_arena_header(base, id, core, flags_config);
  init_arena_stats(arena_base);
  init_lock_free_stack(arena_base);
  SuperBlockList *active_list{init_super_block_list(
      arena_base->arena_base, arena_base->super_block_active_offset)};
  arena_base->active = active_list;
  SuperBlockList *partial_list{init_super_block_list(
      arena_base->arena_base, arena_base->super_block_partial_offset)};
  SuperBlockList *full_list{init_super_block_list(
      arena_base->arena_base, arena_base->super_block_full_offset)};
  arena_base->full = full_list;
  arena_header = arena_base;
};

void Arena::set_super_block_chunk_head(SuperBlockChunk *arena_chunk) {
  if (!head_super_block) {
    head_super_block = arena_chunk;
  } else {
    arena_chunk->next = head_super_block;
    head_super_block = arena_chunk;
  }
};

void Arena::set_medium_chunk_head(MediumChunk *arena_chunk) {
  if (!head_medium_chunk) {
    head_medium_chunk = arena_chunk;
  } else {
    arena_chunk->next = head_medium_chunk;
    head_medium_chunk = arena_chunk;
  }
};

void Arena::alloc_super_block_chunk() {
  void *base{init_memory(default_arena_size * 1024, os_api::PAGE_SIZE)};
  SuperBlockHeader *super_block_header{
      init_super_block_header(base, arena_header->super_block_count)};
  arena_header->super_block_count += 1;
  init_extent_manager(super_block_header->base,
                      super_block_header->extent_manager_offset,
                      super_block_header->extent_manager_pool_offset,
                      super_block_header->usable_region_offset,
                      super_block_header->total_usable_size);
  init_super_block_pool(super_block_header);
  init_super_block_classes(super_block_header);
  SuperBlockChunk *super_block_chunk{init_super_block_chunk(
      super_block_header->base, super_block_header->super_block_chunk_offset)};
  set_super_block_chunk_head(super_block_chunk);
};

void Arena::alloc_medium_chunk() {
  void *base{init_memory(default_arena_size * 1024, os_api::PAGE_SIZE)};
  MediumChunkHeader *medium_chunk_header{
      init_medium_header(base, arena_header->middle_chunk_count)};
  arena_header->middle_chunk_count++;
  init_extent_manager(medium_chunk_header->base,
                      medium_chunk_header->extent_manager_offset,
                      medium_chunk_header->extent_manager_pool_offset,
                      medium_chunk_header->usable_region_offset,
                      medium_chunk_header->total_usable_size);
  MediumChunk *medium_chunk{init_medium_chunk(
      medium_chunk_header->base, medium_chunk_header->medium_chunk_offset)};
  set_medium_chunk_head(medium_chunk);
};

Arena::Arena(std::uint32_t id, std::uint32_t core, std::uint32_t flags_config) {
  alloc_arena_header(id, core, flags_config);
  alloc_super_block_chunk();
  alloc_medium_chunk();
};
void *Arena::find_medium_chunk(std::size_t size) {
  void *raw;
  if (!head_medium_chunk) {
    alloc_medium_chunk();
  }
  MediumChunk *temp{head_medium_chunk};
  while (temp) {
    if ((temp->header->total_used_size + size) >
        temp->header->total_usable_size) {
      temp = temp->next;
      continue;
    }

    raw = temp->extent_manager->alloc_extent(
        size, reinterpret_cast<std::uintptr_t>(temp->header->base));
    temp->header->total_used_size += size;
    return raw;
  }

  alloc_medium_chunk();
  raw = head_medium_chunk->extent_manager->alloc_extent(
      size, reinterpret_cast<std::uintptr_t>(head_medium_chunk->header->base));
  head_medium_chunk->header->total_used_size += size;
  return raw;
};

void *Arena::alloc(std::size_t id, std::size_t size) {
  // Given the current work done in tcache initially i would be expecting and
  // idx matching the already 17 classes
  // i will work later in the paths that involve alignment and a size bigger
  // also if tcache needs a rework or additional path i will do it later for
  // now assume a simple id matchen a class
  LockFreeStack *lock_free_stack{get_lock_free_stack_pointer(arena_header)};
  LockFreeStack::node *lock_node{lock_free_stack->pop()};
  while (lock_node) {
    LockFreeStack::node *next = lock_node->next;
    void *raw{reinterpret_cast<void *>(lock_node)};
    free(raw);
    lock_node = next;
  }

  void *raw;
  if (size >= MAX_CLASS_SIZE) {
    raw = find_medium_chunk(size);
    set_bytes_allocated(size, true, arena_header);
    return raw;
  }
  SuperBlock *super_block{arena_header->active->list[id]};
  if (!super_block->is_full()) {
    raw = super_block->allocate_atomic_span(0);
    set_bytes_allocated(size, false, arena_header);
    return raw;
  }
  super_block->next = arena_header->full->list[id];
  arena_header->full->list[id] = super_block;

  if (super_block && !super_block->is_full()) {
    // i dont know what to do with the hint or where should that come from
    // also this is retuning just  a slot a chunk of the requesten memory
    // should i retun a batch for the tcache if so i need to rethink alloc
    // from the super block
    raw = super_block->allocate_atomic_span(0);
    set_bytes_allocated(size, false, arena_header);
    return raw;
  }
  ArenaStats *arena_stats{
      reinterpret_cast<ArenaStats *>(get_arena_stats(head->header))};
  bool has_space{has_arena_space(arena_stats)};

  if (!has_space)
    return nullptr;

  if (!has_super_block_space(arena_stats))
    return nullptr;

  super_block = alloc_super_block(id, head->header, head->extent_manager);
  if (!super_block)
    return nullptr;
  raw = super_block->allocate_atomic_span(0);
  set_bytes_allocated(size, false, head->header);
  return raw;
};

bool Arena::free(void *ptr) {
  // Here i need to make a distinction between memory managed by the
  // superblock and memory managed fron the extend purely since some requests
  // might exceed the max chuk size of teh block
  std::uintptr_t ptr_addr{reinterpret_cast<std::uintptr_t>(ptr)};

  if (!is_range_arena(ptr_addr, head->header))
    return false;

  SuperBlock *super_block{find_super_block(ptr_addr, head->header)};

  if (super_block) {
    set_bytes_freed_super_block(super_block->get_slot_size(), head->header);
    super_block->free_atomic_span(ptr);
    return true;
  }

  void *base_header{ExtentManager::get_base_header(ptr_addr)};
  set_bytes_freed_extent(ptr, head->header);
  head->extent_manager->free_extent(base_header);

  return true;
};

void Arena::free_remote(void *ptr) {
  LockFreeStack *lock_free_stack{get_lock_free_stack_pointer(head->header)};
  lock_free_stack->push(ptr);
};

bool Arena::is_range_arena(std::uintptr_t ptr, ArenaHeader *header) {
  std::uintptr_t start{reinterpret_cast<std::uintptr_t>(header->arena_base) +
                       header->usable_region_offset};
  std::uintptr_t end{start + header->total_usable_size};
  return start <= ptr && ptr < end;
}
