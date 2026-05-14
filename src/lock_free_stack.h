#pragma once

#include <atomic>
#include <cstdint>

class LockFreeStack {
public:
  struct node {
    node *next;
  };
  std::atomic<node *> head{nullptr};
  void push(void *ptr);
  node *pop();
};
