#include <atomic>
#include <cstdint>
class LockFreeStack {
private:
  struct node {
    std::uintptr_t addr;
    node *next;
    node(std::uintptr_t _addr) : addr(_addr) {}
  };
  std::atomic<node *> head;

public:
  void push(std::uintptr_t addr);
  std::uintptr_t pop();
  node *pop_all();
};
