#include "extent_manager.h"

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

void *ExtentManager::alloc_extent(std::size_t size_node) {
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

  if (entry->size == size_node) {
    void *addr{reinterpret_cast<void *>(entry->base)};
    erase(entry->base);
    free_pool_node(entry);
    return addr;
  } else if (entry->size > size_node) {
    std::uintptr_t allocated_addr{entry->base};
    std::uintptr_t new_base{entry->base + size_node};
    std::size_t remaining_size{entry->size - size_node};
    entry->size = remaining_size;
    entry->base = new_base;
    return reinterpret_cast<void *>(allocated_addr);
  }

  return nullptr;
}

void ExtentManager::free_extent(std::uintptr_t base, std::size_t size) {
  // 0 variables init
  Entry *prev{nullptr};
  Entry *next{nullptr};
  Entry *entry{nullptr};

  entry = insert_new_node(base, size);
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
