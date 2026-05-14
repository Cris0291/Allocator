#include "lock_free_stack.h"
#include <atomic>
#include <new>

void LockFreeStack::push(void *ptr) {
  node *new_node{reinterpret_cast<node *>(ptr)};
  new_node->next = head.load(std::memory_order_relaxed);
  while (!head.compare_exchange_weak(new_node->next, new_node,
                                     std::memory_order_release))
    ;
};

LockFreeStack::node *LockFreeStack::pop() {
  node *node_list{head.exchange(nullptr, std::memory_order_acquire)};
  return node_list;
  // this will be handled at the arena level since it is the onlt consumer
};
