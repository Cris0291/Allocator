#include "red_black_tree.hpp"
#include <cstddef>
#include <shared_mutex>

template <typename K, typename V> class concurrentRBTree {
public:
  struct Entry {
    K key;
    V value;
    RBNode rb;
  };

private:
  RBRoot root{nullptr};
  std::shared_mutex _mutex;

  static Entry *entry_of(RBNode *rb_node) {
    return reinterpret_cast<Entry *>(reinterpret_cast<char *>(rb_node) -
                                     offsetof(Entry, rb_node));
  };

  static const Entry *entry_of(const RBNode *rb_node) {
    return reinterpret_cast<const Entry *>(
        reinterpret_cast<const char *>(rb_node) - offsetof(Entry, rb_node));
  };

public:
  concurrentRBTree() = default;
  ~concurrentRBTree() {
    RBNode *rb_node = rb_first_postorder(&root);
    while (rb_node) {
      RBNode *rb_next_node = rb_next_postorder(rb_node);
      delete entry_of(rb_node);
      rb_node = rb_next_node;
    }
  };
  concurrentRBTree(const concurrentRBTree &) = delete;
  concurrentRBTree &operator=(const concurrentRBTree &) = delete;

  bool insert(const K &key, const V &value) {
    RBNode **link = &root.rb_root;
    RBNode *parent;
    Entry *existing;

    while (*link) {
      parent = *link;
      existing = entry_of(parent);

      if (key < existing->key) {
        link = &parent->left;
      } else if (key > existing->key) {
        link = &parent->right;
      } else {
        existing->value = value;
        return false;
      }
    }

    // Need to change this line since i cannot use new in my allocator
    Entry en = Entry{};
  };
};
