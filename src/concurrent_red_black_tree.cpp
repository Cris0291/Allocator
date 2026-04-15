#include "red_black_tree.hpp"
#include <cstddef>
#include <mutex>
#include <new>
#include <optional>
#include <shared_mutex>

// Most likely i will change this beciuase i dont really know where this fits in
// my arena class
struct DefaultAllocator {
  void *allocate(std::size_t size) { return ::operator new(size); };
  void deallocate(void *ptr) { ::operator delete(ptr); }
};

template <typename K, typename V, typename Allocator = DefaultAllocator>
class concurrentRBTree {
public:
  struct Entry {
    K key;
    V value;
    RBNode rb;
  };

private:
  RBRoot root{nullptr};
  mutable std::shared_mutex _mutex;
  Allocator _allocator;

  static Entry *entry_of(RBNode *rb_node) {
    return reinterpret_cast<Entry *>(reinterpret_cast<char *>(rb_node) -
                                     offsetof(Entry, rb_node));
  };

  static const Entry *entry_of(const RBNode *rb_node) {
    return reinterpret_cast<const Entry *>(
        reinterpret_cast<const char *>(rb_node) - offsetof(Entry, rb_node));
  };

  Entry *create_entry(const K &key, const V &value) {
    void *mem{_allocator.allocate(sizeof(Entry))};
    return new (mem) Entry{key, value, {}};
  };

  void destroy_entry(Entry *entry) {
    entry->~Entry();
    _allocator.deallocate(entry);
  };

public:
  concurrentRBTree() : _allocator(){};
  explicit concurrentRBTree(Allocator alloc) : _allocator(std::move(alloc)){};

  ~concurrentRBTree() {
    RBNode *rb_node = rb_first_postorder(&root);
    while (rb_node) {
      RBNode *rb_next_node = rb_next_postorder(rb_node);
      destroy_entry(entry_of(rb_node));
      rb_node = rb_next_node;
    }
  };
  concurrentRBTree(const concurrentRBTree &) = delete;
  concurrentRBTree &operator=(const concurrentRBTree &) = delete;

  Allocator &get_allocator() { return &_allocator; }

  bool insert(const K &key, const V &value) {
    std::unique_lock<std::shared_mutex> lock(_mutex);

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
    Entry *entry = create_entry(key, value);
    rb_link_node(&entry->rb, parent, link);
    rb_insert_color(&entry->rb, &root.rb_root);
    return true;
  }

  bool erase(const K &key) {
    std::unique_lock<std::shared_mutex> lock(_mutex);

    RBNode *node{root.rb_root};
    while (node) {
      Entry *entry = entry_of(node);
      if (key > entry->key) {
        node = node->right;
      } else if (key < entry->key) {
        node = node->left;
      } else {
        rb_erase(node, &root);
        destroy_entry(node);
        return true;
      }
    }
    return false;
  }

  std::optional<V> find(const K &key) {
    std::shared_lock<std::shared_mutex> lock(_mutex);

    RBNode *node{root.rb_root};

    while (node) {
      Entry *entry{entry_of(node)};
      if (key > entry->right) {
        node = node->right;
      } else if (key < entry->key) {
        node = node->left;
      } else {
        return entry->value;
      }
    }

    return std::nullopt;
  }

  bool contains(const K &key) { return find(key).has_value(); }

  std::optional<V> min(const K &key) const {
    std::shared_lock<std::shared_mutex> lock(_mutex);
    RBNode *node = rb_first(&root);
    if (!node)
      return std::nullopt;
    return entry_of(node)->value;
  }

  std::optional<V> max(const K &key) const {
    std::shared_lock<std::shared_mutex> lock(_mutex);
    RBNode *node = rb_last(&root);
    if (!node)
      return std::nullopt;
    return entry_of(node)->value;
  }
};
