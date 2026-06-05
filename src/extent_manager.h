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
    Entry *next_free{nullptr};
  };

private:
  static constexpr std::size_t ALIGNED_BASE{64 * 1024};
  RBRoot root{nullptr};
  Entry *head{nullptr};
  std::uintptr_t base_pointer;
  std::uintptr_t bump_pointer;
  std::size_t capacity;

  struct ExtentHeader {
    std::size_t size;
    std::uintptr_t base;
  };

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

  inline static void *include_header(std::uintptr_t base_header,
                                     std::size_t size,
                                     std::uintptr_t base_chunk) {

    std::uintptr_t base{base_header + sizeof(ExtentHeader)};
    ExtentHeader *extent_header{new (reinterpret_cast<void *>(base_header))
                                    ExtentHeader{}};
    extent_header->size = size;
    extent_header->base = base_chunk;
    return reinterpret_cast<void *>(base);
  };

  Entry *find_node(std::size_t size);
  std::uintptr_t align_up(std::uintptr_t x, std::size_t size);

  Entry *alloc_pool_node();
  void free_pool_node(Entry *entry);
  Entry *insert_new_node(std::uintptr_t new_base, std::size_t size);
  void erase(std::uintptr_t key);

public:
  static constexpr std::size_t HEADER_SIZE{sizeof(ExtentHeader)};
  ExtentManager(std::uintptr_t pool_base, std::size_t pool_size,
                std::uintptr_t usable_base, std::size_t usable_size);
  void *alloc_extent(std::size_t size, std::uintptr_t base_chunk);

  void *alloc_extent_aligned(std::size_t size);

  void free_extent(void *base_header);

  inline static void *get_base_header(std::uintptr_t base) {
    return reinterpret_cast<void *>(base - sizeof(ExtentHeader));
  }

  inline static std::size_t get_header_size(void *ptr) {
    std::uintptr_t ptr_addr{reinterpret_cast<std::uintptr_t>(ptr)};
    ExtentHeader *header{
        reinterpret_cast<ExtentHeader *>(ptr_addr - sizeof(ExtentHeader))};
    return header->size;
  }
};
