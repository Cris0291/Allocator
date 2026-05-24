#include "extent_manager.h"
#include <cstddef>

ExtentManager::Entry *ExtentManager::alloc_pool_node() {
  Entry *entry{nullptr};
  // First check if there are nodes in the free list
  if (head) {
    entry = head;
    head = head->next_free;
    entry->next_free = nullptr;
  } else if (bump_pointer < capacity) {
    entry = create_entry(bump_pointer);
    bump_pointer += sizeof(Entry);
  }
  // In this case it is possible to return nullptr indicating thate there was
  // no space left
  return entry;
}

void ExtentManager::free_pool_node(Entry *entry) {
  entry->next_free = head;
  head = entry;
}

ExtentManager::ExtentManager(std::uintptr_t pool_base, std::size_t pool_size,
                             std::uintptr_t usable_base,
                             std::size_t usable_size)
    : base_pointer(pool_base), capacity(pool_base + pool_size),
      bump_pointer(pool_base) {
  Entry *entry{alloc_pool_node()};
  entry->base = usable_base;
  entry->size = usable_size;
  entry->rb = {};
  rb_link_node(&entry->rb, nullptr, &root.rb_root);
  rb_insert_color(&entry->rb, &root);
}

ExtentManager::Entry *ExtentManager::find_node(std::size_t size_node) {
  RBNode *node{rb_first(&root)};
  Entry *entry{nullptr};
  while (node) {
    entry = entry_of(node);
    if (entry->size >= size_node)
      break;
    node = rb_next(node);
  }
  if (!node)
    return nullptr;

  return entry;
};

void *ExtentManager::alloc_extent(std::size_t size) {
  std::size_t size_node{size + sizeof(ExtentHeader)};
  Entry *entry{find_node(size_node)};
  if (!entry)
    return nullptr;

  if (entry->size == size_node) {
    void *base{ExtentManager::include_header(entry->base, size)};
    erase(entry->base);
    free_pool_node(entry);
    return base;
  } else if (entry->size > size_node) {
    std::uintptr_t allocated_addr{entry->base};
    std::uintptr_t new_base{entry->base + size_node};
    std::size_t remaining_size{entry->size - size_node};
    entry->size = remaining_size;
    entry->base = new_base;
    void *base{ExtentManager::include_header(allocated_addr, size)};
    return base;
  }

  return nullptr;
}

void *ExtentManager::alloc_extent_aligned(std::size_t size) {
  std::size_t size_node{size + sizeof(ExtentHeader) + ALIGNED_BASE - 1};
  Entry *entry{find_node(size_node)};
  if (!entry)
    return nullptr;

  std::uintptr_t aligned_base{
      align_up(entry->base + sizeof(ExtentHeader), ALIGNED_BASE)};
  std::uintptr_t aligned_base_header{aligned_base - sizeof(ExtentHeader)};
  std::uintptr_t prefix{aligned_base_header - entry->base};
  std::uintptr_t suffix{(entry->base + entry->size) - (aligned_base + size)};

  void *base{ExtentManager::include_header(aligned_base_header, size)};

  if (prefix == 0 && suffix == 0) {
    erase(entry->base);
    free_pool_node(entry);
  } else if (prefix > 0 && suffix == 0) {
    entry->size = prefix;
  } else if (prefix == 0 && suffix > 0) {
    entry->base = aligned_base + size;
    entry->size = suffix;
  } else {
    entry->size = prefix;
    insert_new_node(aligned_base + size, suffix);
  }

  return base;
};

void ExtentManager::free_extent(void *base_header) {
  ExtentHeader *header{reinterpret_cast<ExtentHeader *>(base_header)};
  std::size_t size{header->size};

  Entry *prev{nullptr};
  Entry *next{nullptr};
  Entry *entry{nullptr};

  std::size_t size_with_header{size + sizeof(ExtentHeader)};
  entry = insert_new_node(reinterpret_cast<std::uintptr_t>(base_header),
                          size_with_header);
  RBNode *next_node{&entry->rb};
  next_node = rb_next(next_node);
  RBNode *prev_node{&entry->rb};
  prev_node = rb_prev(prev_node);

  if (next_node)
    next = entry_of(next_node);
  if (prev_node)
    prev = entry_of(prev_node);

  bool merge_prev{prev && (prev->base + prev->size == entry->base)};
  bool merge_next{next && (entry->base + entry->size == next->base)};

  if (merge_prev && merge_next) {
    prev->size += entry->size + next->size;
    rb_erase(&entry->rb, &root);
    rb_erase(&next->rb, &root);
    free_pool_node(entry);
    free_pool_node(next);
  } else if (merge_prev) {
    prev->size += entry->size;
    rb_erase(&entry->rb, &root);
    free_pool_node(entry);
  } else if (merge_next) {
    entry->size += next->size;
    rb_erase(&next->rb, &root);
    free_pool_node(next);
  }
}

ExtentManager::Entry *ExtentManager::insert_new_node(std::uintptr_t new_base,
                                                     std::size_t size) {
  RBNode **link{&root.rb_root};
  RBNode *parent{nullptr};
  Entry *existing;

  while (*link) {
    parent = *link;
    existing = entry_of(parent);
    if (new_base > existing->base) {
      link = &parent->right;
    } else {
      link = &parent->left;
    }
  }

  Entry *entry{alloc_pool_node()};
  entry->size = size;
  entry->base = new_base;
  rb_link_node(&entry->rb, parent, link);
  rb_insert_color(&entry->rb, &root);
  return entry;
}

void ExtentManager::erase(std::uintptr_t key) {
  RBNode *node = root.rb_root;
  Entry *entry;

  while (node) {
    entry = entry_of(node);
    if (entry->base > key) {
      node = node->left;
    } else if (entry->base < key) {
      node = node->right;
    } else {
      // I am not sure if memory could be leak from here i dont relaly think
      // so since the rb is stil in entry which in the case is in linked list
      rb_erase(node, &root);
      break;
    }
  }
}

std::uintptr_t ExtentManager::align_up(std::uintptr_t x, std::size_t size) {
  return (x + (size - 1)) & ~(size - 1);
};
