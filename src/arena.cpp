#include "extent_manager.h"
#include "lock_free_stack.h"
#include "map_size.h"
#include "os_api.h"
#include "super_block.h"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <new>

class Arena {
private:
  static constexpr std::size_t NUM_CLASSES{17};
  // Size of arena is 256 but allocated an extra page for header and metadata
  static constexpr std::size_t default_arena_size{260};

  struct ArenaHeader {
    std::uint32_t version;
    std::uint32_t magic;
    std::uint32_t flags;
    std::uint16_t arena_id;
    std::uint16_t owner_core;
    std::uint64_t total_usable_size;
    std::uintptr_t arena_stats_offset;
    std::uintptr_t super_block_health_offset;
    std::uintptr_t super_block_pool_offset;
    std::uintptr_t super_block_classes_offset;
    std::uintptr_t extent_manager_offset;
    std::uintptr_t extent_manager_pool_offset;
    std::uintptr_t lock_free_stack_offset;
    std::uintptr_t usable_region_offset;
    void *arena_base;
  };

  struct ArenaStats {
    std::atomic_uint_fast64_t alloc_count;
    std::atomic_uint_fast64_t free_count;
    std::atomic_uint_fast64_t bytes_allocated;
    std::atomic_uint_fast64_t bytes_free;
    std::atomic_uint_fast64_t bytes_in_use;
    std::atomic_uint_fast64_t bytes_wasted;
    std::uint8_t padding[(64 - ((6 * sizeof(uint64_t)) % 64)) % 64];
  };

  struct SuperBlockHealth {
    std::atomic_uint_fast64_t superblocks_active;
    std::atomic_uint_fast64_t superblocks_full;
    std::atomic_uint_fast64_t superblocks_created;
    std::atomic_uint_fast64_t superblocks_released;
    std::uint8_t padding[(64 - ((4 * sizeof(uint64_t)) % 64)) % 64];
  };

  struct SuperBlockPool {
    alignas(SuperBlock) unsigned char storage[4][sizeof(SuperBlock)];
    bool occupied[4]{};
  };

  struct SuperBlockPoolClasses {
    SuperBlock *super_block_pool_classes[NUM_CLASSES]{};
  };

  ExtentManager *extent_manager;

  struct ExtentManagerPool {
    ExtentManager::Entry entry_pool[64];
  };

  ArenaHeader *base;

  inline static std::uintptr_t align_up(std::uintptr_t x, std::size_t size) {
    return (x + (size - 1)) & ~(size - 1);
  };

  static ArenaHeader *init_header(void *base, std::uint32_t id,
                                  std::uint32_t core,
                                  std::uint32_t flags_config) {
    ArenaHeader *arena_base{reinterpret_cast<ArenaHeader *>(base)};
    arena_base->arena_id = id;
    arena_base->owner_core = core;
    arena_base->arena_base = base;
    arena_base->flags = flags_config;
    arena_base->version = 1;
    arena_base->magic = 0xA17ECAFE;

    std::uintptr_t base_ptr{reinterpret_cast<std::uintptr_t>(base)};
    std::uintptr_t header_offset{sizeof(ArenaHeader)};
    std::uintptr_t header_alignup_offset{align_up(header_offset, 64)};

    // Offset   Size   Description
    //-----    ----   -----------
    // 0         96    ArenaHeader(64 bit)
    // 96        32    Padding for the next cache line
    // 128       64    ArenaStats
    // 192       64    SuperBlockHealth
    // 256       192   SuperBlockPool
    // 448       136   SuperBlockClasses
    // 592       40    ExtentManager
    // 632      3072   Extent manager pool
    // 3704      8     Lock free stack
    // 3710     386         Padding to usable memory
    // 4096
    arena_base->arena_stats_offset = header_alignup_offset;
    arena_base->super_block_health_offset =
        arena_base->arena_stats_offset + sizeof(ArenaStats);
    arena_base->super_block_pool_offset =
        arena_base->super_block_health_offset + sizeof(SuperBlockHealth);
    arena_base->super_block_classes_offset =
        arena_base->super_block_pool_offset + sizeof(SuperBlockPool);
    arena_base->extent_manager_offset =
        arena_base->super_block_classes_offset + sizeof(SuperBlockPoolClasses);
    arena_base->extent_manager_pool_offset =
        arena_base->extent_manager_offset + sizeof(ExtentManager);
    arena_base->lock_free_stack_offset =
        arena_base->extent_manager_pool_offset + sizeof(LockFreeStack);

    arena_base->usable_region_offset =
        align_up(arena_base->lock_free_stack_offset, 4096);

    arena_base->total_usable_size =
        (default_arena_size * 1024) - arena_base->usable_region_offset;

    return arena_base;
  };

  static void init_arena_stats(ArenaHeader *header) {
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

  void init_extent_manager(ArenaHeader *header) {
    void *extent_manager_addr = reinterpret_cast<void *>(
        reinterpret_cast<std::uintptr_t>(header->arena_base) +
        header->extent_manager_offset);

    std::uintptr_t arena_base{
        reinterpret_cast<std::uintptr_t>(header->arena_base)};

    std::uintptr_t pool_base{arena_base + header->extent_manager_pool_offset};
    std::size_t pool_size{sizeof(ExtentManagerPool)};
    std::uintptr_t usable_base{arena_base + header->usable_region_offset};
    std::size_t usable_size{header->total_usable_size};

    extent_manager = new (extent_manager_addr)
        ExtentManager(pool_base, pool_size, usable_base, usable_size);
  };

  void init_super_block_pool() {
    void *super_block_pool_addr{get_super_block_pool()};
    new (super_block_pool_addr) SuperBlockPool();
  };

  void init_super_block_classes() {
    void *super_block_classes_addr{get_super_block_classes()};
    new (super_block_classes_addr) SuperBlockPoolClasses();
  };

  void init_super_block_health() {
    void *super_block_health_addr{get_super_block_health_address()};
    new (super_block_health_addr) SuperBlockHealth();
  };

  void init_lock_free_stack() {
    void *lock_free_stack_addr{get_lock_free_stack()};
    new (lock_free_stack_addr) LockFreeStack();
  };

  void *get_arena_stats() {
    return reinterpret_cast<void *>(
        reinterpret_cast<std::uintptr_t>(base->arena_base) +
        base->arena_stats_offset);
  };

  void *get_super_block_pool() {
    return reinterpret_cast<void *>(
        reinterpret_cast<std::uintptr_t>(base->arena_base) +
        base->super_block_pool_offset);
  };

  void *get_super_block_health_address() {
    return reinterpret_cast<void *>(
        reinterpret_cast<std::uintptr_t>(base->arena_base) +
        base->super_block_health_offset);
  };

  void *get_super_block_classes() {
    return reinterpret_cast<void *>(
        reinterpret_cast<std::uintptr_t>(base->arena_base) +
        base->super_block_classes_offset);
  };

  void *get_extent_manager_pool() {
    return reinterpret_cast<void *>(
        reinterpret_cast<std::uintptr_t>(base->arena_base) +
        base->extent_manager_pool_offset);
  };

  void *get_lock_free_stack() {
    return reinterpret_cast<void *>(
        reinterpret_cast<std::uintptr_t>(base->arena_base) +
        base->lock_free_stack_offset);
  };

  bool has_arena_space(ArenaStats *arena_stats) {
    return base->total_usable_size >
           arena_stats->bytes_allocated.load(std::memory_order_acquire);
  };

  bool has_super_block_space(ArenaStats *arena_stats) {
    return base->total_usable_size -
               arena_stats->bytes_allocated.load(std::memory_order_acquire) >=
           SuperBlock::get_super_block_size();
  }

  SuperBlock *alloc_super_block(std::size_t id) {
    // Have doubst in here
    // flow should be load super_block_health_addr->superblocks_active
    // then check if at that moment they are less than 4 if so proceed since we
    // can only have 4 of then i wonder if super bclok pool should be atomic in
    // iorder to see if a super block for that size is alrady here then recheck
    // for 4 and for empty slot and if so allocate
    SuperBlock *super_block{nullptr};
    void *super_block_pool_addr{get_super_block_pool()};

    SuperBlockPool *super_block_pool{
        reinterpret_cast<SuperBlockPool *>(super_block_pool_addr)};

    void *super_block_pool_classes_offset{get_super_block_classes()};

    SuperBlockPoolClasses *super_block_pool_classes{
        reinterpret_cast<SuperBlockPoolClasses *>(
            super_block_pool_classes_offset)};

    for (int i{}; i < 4; i++) {
      if (super_block_pool->occupied[i])
        continue;

      void *super_block_addr{
          reinterpret_cast<void *>(super_block_pool->storage[i])};
      super_block = new (super_block_addr)
          SuperBlock(base->arena_id, *extent_manager, map_info[id].size);
      super_block_pool_classes->super_block_pool_classes[id] = super_block;
      super_block_pool->occupied[i] = true;
      ArenaStats *stats{reinterpret_cast<ArenaStats *>(get_arena_stats())};
      stats->bytes_allocated.fetch_add(SuperBlock::get_super_block_size(),
                                       std::memory_order_release);
      break;
    }

    return super_block;
  };

  SuperBlock *find_super_block(std::uintptr_t ptr) {
    // potential refactor with previous function
    // maybe add a template to accept a lanbda function the idea is
    // to be able to dins a super block on different consitions we will see
    SuperBlock *super_block{nullptr};
    void *super_block_pool_addr{get_super_block_pool()};

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

  SuperBlock *get_super_block_class_or_null(std::size_t id) {
    void *super_block_classes{get_super_block_classes()};
    SuperBlockPoolClasses *pool_classes{
        reinterpret_cast<SuperBlockPoolClasses *>(super_block_classes)};
    SuperBlock **classes_array{pool_classes->super_block_pool_classes};
    if (classes_array[id])
      return classes_array[id];
    return nullptr;
  }

  bool is_range_arena(std::uintptr_t ptr) {
    std::uintptr_t start{reinterpret_cast<std::uintptr_t>(base->arena_base) +
                         base->usable_region_offset};
    std::uintptr_t end{start + base->total_usable_size};
    return start <= ptr && ptr < end;
  }

  ArenaStats *get_arena_stats_pointer() {
    void *arena_stats_addr{get_arena_stats()};
    ArenaStats *arena_stats{reinterpret_cast<ArenaStats *>(arena_stats_addr)};
    return arena_stats;
  }

  LockFreeStack *get_lock_free_stack_pointer() {
    void *lock_free_stack_addr{get_lock_free_stack()};
    return reinterpret_cast<LockFreeStack *>(lock_free_stack_addr);
  };

  void set_bytes_allocated(std::size_t size, bool is_extent) {
    std::size_t final_size{is_extent ? size + ExtentManager::HEADER_SIZE
                                     : size};
    ArenaStats *arena_stats{get_arena_stats_pointer()};
    arena_stats->bytes_allocated.fetch_add(final_size,
                                           std::memory_order_release);
  }

  void set_bytes_freed_extent(void *ptr) {
    ArenaStats *arena_stats{get_arena_stats_pointer()};
    std::size_t size{ExtentManager::get_header_size(ptr)};
    arena_stats->bytes_free.fetch_add(size + ExtentManager::HEADER_SIZE,
                                      std::memory_order_release);
  }

  void set_bytes_freed_super_block(std::size_t size) {
    ArenaStats *arena_stats{get_arena_stats_pointer()};
    arena_stats->bytes_free.fetch_add(size, std::memory_order_release);
  }

public:
  Arena(std::uint32_t id, std::uint32_t core, std::uint32_t flags_config) {
    std::size_t arena_bytes{default_arena_size * 1024};
    os_api::MemSpan mem_info;

    os_api::reserve_address_space(arena_bytes, os_api::PAGE_SIZE, mem_info);

    void *base{mem_info.addr};

    os_api::commit_memory(base, 4096);

    ArenaHeader *arena_base = init_header(base, id, core, flags_config);
    this->base = arena_base;
    init_arena_stats(arena_base);
    init_extent_manager(arena_base);
    init_super_block_health();
    init_super_block_pool();
    init_super_block_classes();
    init_lock_free_stack();
  };
  void *alloc(std::size_t id, std::size_t size) {
    // Given the current work done in tcache initially i would be expecting and
    // idx matching the already 17 classes
    // i will work later in the paths that involve alignment and a size bigger
    // also if tcache needs a rework or additional path i will do it later for
    // now assume a simple id matchen a class
    LockFreeStack *lock_free_stack{get_lock_free_stack_pointer()};
    LockFreeStack::node *lock_node{lock_free_stack->pop()};
    while (lock_node) {
      void *raw{reinterpret_cast<void *>(lock_node)};
      free(raw);
      lock_node = lock_node->next;
    }

    void *raw;
    if (size >= MAX_CLASS_SIZE) {
      raw = extent_manager->alloc_extent(size);
      if (raw)
        set_bytes_allocated(size, true);
      return raw;
    }
    SuperBlock *super_block{get_super_block_class_or_null(id)};
    if (super_block && !super_block->is_full()) {
      // i dont know what to do with the hint or where should that come from
      // also this is retuning just  a slot a chunk of the requesten memory
      // should i retun a batch for the tcache if so i need to rethink alloc
      // from the super block
      raw = super_block->allocate_atomic_span(0);
      set_bytes_allocated(size, false);
      return raw;
    }
    ArenaStats *arena_stats{reinterpret_cast<ArenaStats *>(get_arena_stats())};
    bool has_space{has_arena_space(arena_stats)};

    if (!has_space)
      return nullptr;

    if (!has_super_block_space(arena_stats))
      return nullptr;

    super_block = alloc_super_block(id);
    if (!super_block)
      return nullptr;
    raw = super_block->allocate_atomic_span(0);
    set_bytes_allocated(size, false);
    return raw;
  };

  bool free(void *ptr) {
    // Here i need to make a distinction between memory managed by the
    // superblock and memory managed fron the extend purely since some requests
    // might exceed the max chuk size of teh block
    std::uintptr_t ptr_addr{reinterpret_cast<std::uintptr_t>(ptr)};

    if (!is_range_arena(ptr_addr))
      return false;

    SuperBlock *super_block{find_super_block(ptr_addr)};

    if (super_block) {
      set_bytes_freed_super_block(super_block->get_slot_size());
      super_block->free_atomic_span(ptr);
      return true;
    }

    void *base_header{ExtentManager::get_base_header(ptr_addr)};
    set_bytes_freed_extent(ptr);
    extent_manager->free_extent(base_header);

    return true;
  };

  void free_remote(void *ptr) {
    LockFreeStack *lock_free_stack{get_lock_free_stack_pointer()};
    lock_free_stack->push(ptr);
  };
};
