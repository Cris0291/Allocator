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
  Entry *head;
  Entry *last;
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

  Entry *create_entry(void *base, std::size_t size) {
    Entry *entry{new (base) Entry{
        reinterpret_cast<std::uintptr_t>(base), size, {}, nullptr}};
    return entry;
  }

public:
  void *alloc_node(std::size_t size_node) {
    RBNode *node{nullptr};
    Entry *entry{nullptr};
    while (true) {
      node = rb_first(&root);
      entry = entry_of(node);
      if (entry->size >= size_node)
        break;
      node = rb_next(node);
    }
    if (entry->size == size_node) {
      void *addr{reinterpret_cast<void *>(entry->base)};
      entry->base = 0;
      entry->size = 0;
      last->next_free = entry;
      return addr;
    } else if (entry->size > size_node) {
    }
  }

private:
  Entry *insert_new_node(std::size_t size) {
    RBNode **link{&root.rb_root};
    RBNode *parent;
    Entry *existing;

    while (*link) {
      parent = *link;
      existing = entry_of(parent);
      if (bump_pointer > existing->base) {
        link = &parent->right;
      } else {
        link = &parent->left;
      }
    }

    Entry *entry{create_entry(reinterpret_cast<void *>(bump_pointer), size)};
    rb_link_node(&entry->rb, parent, link);
    rb_insert_color(&entry->rb, &root);
    return entry;
  }
};
