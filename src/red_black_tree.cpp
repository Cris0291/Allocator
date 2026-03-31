enum class RBColor { Red, Black };

// this is based on the kernel version of a rb tree
// see what will happen on 32 bit machine

struct RBNode {
  unsigned long rb_parent_color;
  RBNode *left;
  RBNode *right;
};

struct RBRoot {
  RBNode *root = nullptr;
};

inline RBNode *get_rb_parent(const RBNode *rb_node) {
  return reinterpret_cast<RBNode *>(rb_node->rb_parent_color & ~3UL);
};

inline unsigned long get_rb_color(RBNode *rb_node) {
  return rb_node->rb_parent_color & 1;
};
