#include "red_black_tree.hpp"
#include <cstddef>
#include <cstdint>

class ExtentManager {
public:
  struct Entry {
    std::uintptr_t base;
    std::size_t size;
    RBNode rb;
    Entry *next_free;
  };

private:
  RBRoot root{nullptr};
  Entry *head{nullptr};
  std::uintptr_t base_pointer;
  std::uintptr_t bump_pointer;
  std::size_t capacity;

  static Entry *entry_of(RBNode *rb_node) {
    return reinterpret_cast<Entry *>(reinterpret_cast<char *>(rb_node) -
                                     offsetof(Entry, rb));
  }

  static const Entry *entry_of(const RBNode *rb_node) {
    return reinterpret_cast<const Entry *>(
        reinterpret_cast<const char *>(rb_node) - offsetof(Entry, rb));
  }

  static Entry *create_entry(std::uintptr_t base) {
    return new (reinterpret_cast<void *>(base)) Entry{};
  }

  Entry *find_prev_free(std::uintptr_t base) {
    RBNode *node{rb_first(&root)};
    Entry *entry{nullptr};
    while (node) {
      entry = entry_of(node);
      if ((entry->base + entry->size) == base)
        return entry;
      node = rb_next(node);
    }
    return nullptr;
  }

  Entry *alloc_pool_node() {
    Entry *entry;
    // First check if there are nodes in the free list
    if (head) {
      entry = head;
      head = head->next_free;
      entry->next_free = nullptr;
      return entry;
    } else if (bump_pointer < capacity) {
      entry = create_entry(bump_pointer);
      bump_pointer += sizeof(Entry);
    }
    // In this case it is possible to return nullptr indicating thate there was
    // no space left
    return entry;
  }

  void free_pool_node(Entry *entry) {
    entry->next_free = head;
    head = entry;
  }

public:
  ExtentManager(std::uintptr_t pool_base, std::size_t pool_size,
                std::uintptr_t usable_base, std::size_t usable_size)
      : base_pointer(pool_base), capacity(pool_base + pool_size),
        bump_pointer(pool_base) {
    Entry *entry{alloc_pool_node()};
    entry->base = usable_base;
    entry->size = usable_size;
    entry->rb = {};
    rb_link_node(&entry->rb, nullptr, &root.rb_root);
    rb_insert_color(&entry->rb, &root);
  }

  void *alloc_extent(std::size_t size_node) {
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
    } else {
      // no size left return nullptr let arena handle
      return nullptr;
    }
  }

  bool free_extent(std::uintptr_t base, std::size_t size) {
    // 0 variables init
    Entry *prev{nullptr};
    Entry *next{nullptr};
    Entry *entry{nullptr};
    // 1 Go first then next in ordert to find prev region
    prev = find_prev_free(base);
    // 2 if prev region find sucessor from that node if not go last and prev

    // 3 if found eiither just change one node if just one region if both change
    // one delete the there 4 if neither region were found allocate a single
    // node
  }

private:
  Entry *insert_new_node(std::uintptr_t new_base, std::size_t size) {
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

  void erase(std::uintptr_t key) {
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
};
