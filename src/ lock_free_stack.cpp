#include "lock_free_stack.h"
#include <atomic>
#include <new>

void LockFreeStack::push(std::uintptr_t addr) {
  // this nodes will live in a dedicated memory on the header of each arena
  // get arena memory
  void *raw;
  LockFreeStack::node *new_node{new (raw) LockFreeStack::node(addr)};
  new_node->next = head.load(std::memory_order_acquire);
  while (!head.compare_exchange_weak(new_node->next, new_node))
    ;
};

std::uintptr_t LockFreeStack::pop() {
  LockFreeStack::node *old_head = head.load(std::memory_order_acquire);
  while (old_head && !head.compare_exchange_weak(old_head, old_head->next))
    ;
  return old_head->addr;
};

LockFreeStack::node *LockFreeStack::pop_all() {
  LockFreeStack::node *nodes{head.exchange(nullptr)};
  return nodes;
};
