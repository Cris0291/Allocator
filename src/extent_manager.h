#pragma once

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

  inline static Entry *entry_of(RBNode *rb_node) {
    return reinterpret_cast<Entry *>(reinterpret_cast<char *>(rb_node) -
                                     offsetof(Entry, rb));
  };

  inline static const Entry *entry_of(const RBNode *rb_node) {
    return reinterpret_cast<const Entry *>(
        reinterpret_cast<const char *>(rb_node) - offsetof(Entry, rb));
  };

  inline static Entry *create_entry(std::uintptr_t base) {
    return new (reinterpret_cast<void *>(base)) Entry{};
  };

  Entry *alloc_pool_node();
  void free_pool_node(Entry *entry);
  Entry *insert_new_node(std::uintptr_t new_base, std::size_t size);
  void erase(std::uintptr_t key);

public:
  ExtentManager(std::uintptr_t pool_base, std::size_t pool_size,
                std::uintptr_t usable_base, std::size_t usable_size);
  void *alloc_extent(std::size_t size_node);

  void free_extent(std::uintptr_t base, std::size_t size);
};
