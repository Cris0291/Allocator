#include "red_black_tree.hpp"
#include <cstddef>
#include <cstdint>
class ExtentManager {
public:
  struct Entry {
    std::uintptr_t base;
    std::size_t size;
    RBNode rb;
  };

private:
  struct ExtentNode {
    Entry *entry;
    ExtentNode *next;
  };
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
};
