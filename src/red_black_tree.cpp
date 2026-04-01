enum class RBColor : unsigned long { Red, Black };

// this is based on the kernel version of a rb tree
// see what will happen on 32 bit machine

struct RBNode {
  unsigned long rb_parent_color;
  RBNode *left;
  RBNode *right;
};

struct RBRoot {
  RBNode *rb_root = nullptr;
};

inline RBNode *get_rb_parent(const RBNode *rb_node) {
  return reinterpret_cast<RBNode *>(rb_node->rb_parent_color & ~3UL);
};

inline unsigned long get_rb_color(const RBNode *rb_node) {
  return rb_node->rb_parent_color & 1;
};

inline bool is_rb_red(const RBNode *rb_node) {
  return !(rb_node->rb_parent_color & 1);
};

inline bool is_rb_black(const RBNode *rb_node) {
  return !!(rb_node->rb_parent_color & 1);
};

inline void rb_set_parent_color(RBNode *rb_node, RBNode *parent, int color) {
  rb_node->rb_parent_color = reinterpret_cast<unsigned long>(parent) + color;
};

inline void rb_set_parent(RBNode *rb_node, RBNode *parent) {
  rb_node->rb_parent_color =
      reinterpret_cast<unsigned long>(parent) + get_rb_color(rb_node);
};

inline void rb_set_black(RBNode *rb_node) { rb_node->rb_parent_color |= 1UL; };

inline RBNode *get_rb_red_parent(RBNode *rb_node) {
  return reinterpret_cast<RBNode *>(rb_node->rb_parent_color);
};

inline void rb_change_child(RBNode *old_node, RBNode *new_node, RBNode *parent,
                            RBRoot *root) {
  if (parent) {
    if (parent->left == old_node) {
      parent->left = new_node;
    } else {
      parent->right = new_node;
    }
  } else {
    root->rb_root = new_node;
  }
}
