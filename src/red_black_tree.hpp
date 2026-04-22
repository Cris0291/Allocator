#include <cstddef>
#include <memory>
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
};

inline void rb_rotate_set_parents(RBNode *old_node, RBNode *new_node,
                                  RBRoot *root, int color) {
  RBNode *parent{get_rb_parent(old_node)};
  new_node->rb_parent_color = old_node->rb_parent_color;
  rb_set_parent_color(old_node, new_node, color);
  rb_change_child(old_node, new_node, parent, root);
};

inline void rb_link_node(RBNode *rb_node, RBNode *parent, RBNode **link) {
  rb_node->rb_parent_color = reinterpret_cast<unsigned long>(parent);

  rb_node->left = nullptr;
  rb_node->right = nullptr;

  *link = rb_node;
};

// resolve insert violations
inline void rb_insert_color(RBNode *rb_node, RBRoot *root) {
  RBNode *parent{get_rb_red_parent(rb_node)};
  RBNode *grandparent{};
  RBNode *tmp;

  while (true) {
    // parent does not exist we are at root either because of a push in case 1
    // or tree was empty
    if (!parent) {
      rb_set_parent_color(rb_node, nullptr, static_cast<int>(RBColor::Black));
      break;
    }

    if (is_rb_black(parent))
      break;

    // Parent is red at this point cannot be root and gparent exists and is
    // black
    grandparent = get_rb_red_parent(parent);
    tmp = grandparent->right;

    // parent is left
    if (tmp != parent) {

      if (tmp && is_rb_red(tmp)) {
        // case 1 parent red uncle red and node red
        rb_set_parent_color(tmp, grandparent, static_cast<int>(RBColor::Black));
        rb_set_parent_color(parent, grandparent,
                            static_cast<int>(RBColor::Black));

        rb_node = grandparent;
        parent = get_rb_parent(rb_node);

        rb_set_parent_color(rb_node, parent, static_cast<int>(RBColor::Red));
        continue;
      }
      tmp = parent->right;
      // case 2 parent red node red and right child of parent uncle black
      // this case will leave in a temporary broken state untill case 3
      if (rb_node == tmp) {
        // left rotate node with parent
        tmp = rb_node->left;
        parent->right = tmp;
        rb_node->left = parent;

        if (tmp) {
          rb_set_parent_color(tmp, parent, static_cast<int>(RBColor::Black));
        }

        rb_set_parent_color(parent, rb_node, static_cast<int>(RBColor::Red));

        parent = rb_node;
        tmp = rb_node->right;
      }
      // case 3 could be a stad alone case or fall through directly from case 2
      grandparent->left = tmp;
      parent->right = grandparent;

      if (tmp) {
        rb_set_parent_color(tmp, grandparent, static_cast<int>(RBColor::Black));
      }

      rb_rotate_set_parents(grandparent, parent, root,
                            static_cast<int>(RBColor::Red));
      break;
    } else {
      // Mirror for parent right
      tmp = grandparent->left;
      if (tmp && is_rb_red(tmp)) {
        // Mirror case 1
        rb_set_parent_color(parent, grandparent,
                            static_cast<int>(RBColor::Black));
        rb_set_parent_color(tmp, grandparent, static_cast<int>(RBColor::Black));

        rb_node = grandparent;
        parent = get_rb_parent(rb_node);

        rb_set_parent_color(rb_node, parent, static_cast<int>(RBColor::Red));
        continue;
      }

      tmp = parent->left;

      if (tmp == rb_node) {
        // Mirror case 2
        tmp = rb_node->right;
        parent->left = tmp;
        rb_node->right = parent;

        if (tmp) {
          rb_set_parent_color(tmp, parent, static_cast<int>(RBColor::Black));
        }

        rb_set_parent_color(parent, rb_node, static_cast<int>(RBColor::Red));
        parent = rb_node;
        tmp = rb_node->left;
      }
      // Mirror case 3
      grandparent->right = tmp;
      parent->left = grandparent;

      if (tmp) {
        rb_set_parent_color(tmp, grandparent, static_cast<int>(RBColor::Black));
      }

      rb_rotate_set_parents(grandparent, parent, root,
                            static_cast<int>(RBColor::Red));
      break;
    }
  };
}

inline RBNode *rb_erase_augmented(RBNode *node, RBRoot *root) {
  RBNode *child = node->right;
  RBNode *tmp = node->left;
  RBNode *parent;
  RBNode *rebalance;
  unsigned long pc;

  if (!tmp) {
    // Case 1 node to be deleted only has right child or null
    // This is based on the kernel so i am using the same trick
    // child must be red otherwise it would not be balanced with the left
    // include and parent must be black
    pc = node->rb_parent_color;
    parent = reinterpret_cast<RBNode *>(pc & ~3UL);
    rb_change_child(node, child, parent, root);

    if (child) {
      child->rb_parent_color = pc;
      rebalance = nullptr;
    } else {
      rebalance = (pc & 1) ? parent : nullptr;
    }
  } else if (!child) {
    // Case 1b mirror of the previous
    pc = node->rb_parent_color;
    tmp->rb_parent_color = pc;
    parent = reinterpret_cast<RBNode *>(pc & ~3UL);
    rb_change_child(node, tmp, parent, root);
    rebalance = nullptr;
  } else {
    // Case 2 and 3 node has both left and right
    RBNode *succesor = child;
    RBNode *child2;

    tmp = child->left;
    if (!tmp) {
      parent = succesor;
      child2 = succesor->right;
    } else {
      // Case 3
      do {
        parent = succesor;
        succesor = tmp;
        tmp = tmp->left;
      } while (tmp);

      child2 = succesor->right;

      parent->left = child2;
      succesor->right = child;
      rb_set_parent(child, succesor);
    }
    // Both case 2 and 3 converge here
    tmp = node->left;
    succesor->left = tmp;
    rb_set_parent(tmp, succesor);

    pc = node->rb_parent_color;
    tmp = reinterpret_cast<RBNode *>(pc & ~3UL);
    rb_change_child(node, succesor, tmp, root);

    if (child) {
      rb_set_parent_color(child2, parent, static_cast<int>(RBColor::Black));
      rebalance = nullptr;
    } else {
      rebalance = is_rb_black(succesor) ? parent : nullptr;
    }

    succesor->rb_parent_color = pc;
  }
  return rebalance;
}

inline void rb_erase_color(RBNode *parent, RBRoot *root) {
  RBNode *node = nullptr;
  RBNode *sibling;
  RBNode *tmp1;
  RBNode *tmp2;

  while (true) {
    sibling = parent->right;

    if (node != sibling) {
      // node is on the left subtree
      if (is_rb_red(sibling)) {
        // Case 1 sibling is red
        tmp1 = sibling->left;
        parent->right = tmp1;
        sibling->left = parent;
        rb_set_parent_color(tmp1, parent, static_cast<int>(RBColor::Black));
        rb_rotate_set_parents(parent, sibling, root,
                              static_cast<int>(RBColor::Red));
        sibling = tmp1;
      }
      // Now we test sibling children in order to see case 2,3 or 4
      tmp1 = sibling->right;
      if (!tmp1 || is_rb_black(tmp1)) {
        tmp2 = sibling->left;
        if (!tmp2 || is_rb_black(tmp2)) {
          // Case 2
          rb_set_parent_color(sibling, parent, static_cast<int>(RBColor::Red));

          if (is_rb_red(parent)) {
            rb_set_black(parent);
          } else {
            node = parent;
            parent = get_rb_parent(node);
            if (parent)
              continue;
          }
          break;
        }
        // Case 3 is just preparing case 4 at this point data structure is
        // broken
        tmp1 = tmp2->right;
        sibling->left = tmp1;
        tmp2->right = sibling;
        parent->right = tmp2;

        if (tmp1) {
          rb_set_parent_color(tmp1, sibling, static_cast<int>(RBColor::Black));
        }

        tmp1 = sibling;
        sibling = tmp2;
      }
      // Case 4
      tmp2 = sibling->left;
      parent->right = tmp2;
      sibling->left = parent;

      rb_set_parent_color(tmp1, sibling, static_cast<int>(RBColor::Black));

      if (tmp1) {
        rb_set_parent(tmp2, parent);
      }

      rb_rotate_set_parents(parent, sibling, root,
                            static_cast<int>(RBColor::Black));
    } else {

      // Mirror cases right subtree
      sibling = parent->left;
      if (is_rb_red(sibling)) {
        // Mirror case 1
        tmp1 = sibling->right;
        parent->left = tmp1;
        sibling->right = parent;
        rb_set_parent_color(tmp1, parent, static_cast<int>(RBColor::Black));
        rb_rotate_set_parents(parent, sibling, root,
                              static_cast<int>(RBColor::Red));
        sibling = tmp1;
      }

      tmp1 = sibling->left;
      if (!tmp1 || is_rb_black(tmp1)) {
        tmp2 = sibling->right;
        if (!tmp2 || is_rb_black(tmp2)) {
          // Mirror case 2
          rb_set_parent_color(tmp1, parent, static_cast<int>(RBColor::Red));
          if (is_rb_red(parent)) {
            rb_set_black(parent);
          } else {
            node = parent;
            parent = get_rb_parent(node);
            if (parent)
              continue;
          }
          break;
        }
        // Mirror case 3
        tmp1 = tmp2->left;
        sibling->right = tmp1;
        tmp2->left = sibling;
        parent->left = tmp2;
        if (tmp1)
          rb_set_parent_color(tmp1, sibling, static_cast<int>(RBColor::Black));

        tmp1 = sibling;
        sibling = tmp2;
      }
      // Mirror case 4
      tmp2 = sibling->right;
      parent->left = tmp2;
      sibling->right = parent;
      rb_set_parent_color(tmp1, sibling, static_cast<int>(RBColor::Black));
      if (tmp2)
        rb_set_parent(tmp2, parent);
      rb_rotate_set_parents(parent, sibling, root,
                            static_cast<int>(RBColor::Black));
    }
  }
}

inline void rb_erase(RBNode *rb_node, RBRoot *root) {
  RBNode *rebalance = rb_erase_augmented(rb_node, root);
  if (rebalance)
    rb_erase_color(rebalance, root);
};

// Utitlity functions for empty node and empty root
// An empty node points towards itself
inline bool rb_empty_node(RBNode *rb_node) {
  return rb_node->rb_parent_color == reinterpret_cast<unsigned long>(rb_node);
};

inline void rb_clear_node(RBNode *rb_node) {
  rb_node->rb_parent_color = reinterpret_cast<unsigned long>(rb_node);
};

inline bool rb_empty_root(RBRoot *root) { return root->rb_root == nullptr; };

// Find minimum
inline RBNode *rb_first(const RBRoot *root) {
  RBNode *n = root->rb_root;
  if (!n)
    return nullptr;
  while (n->left)
    n = n->left;
  return n;
};

// Find minimum
inline RBNode *rb_last(const RBRoot *root) {
  RBNode *n = root->rb_root;
  if (!n)
    return nullptr;
  while (n->right)
    n = n->right;
  return n;
};

inline RBNode *rb_next(RBNode *rb_node) {
  RBNode *parent{nullptr};

  if (rb_empty_node(rb_node))
    return nullptr;

  // SUccessor is the leftmost in the right subtree
  if (rb_node->right) {
    RBNode *n = rb_node->right;
    while (n->left) {
      n = n->left;
    }
    return n;
  }

  while ((parent = get_rb_parent(rb_node)) && rb_node == parent->right) {
    rb_node = parent;
  }

  return parent;
};

inline RBNode *rb_prev(RBNode *rb_node) {
  RBNode *parent{nullptr};

  if (rb_empty_node(rb_node))
    return nullptr;

  if (rb_node->left) {
    RBNode *n = rb_node->left;
    while (n->right) {
      n = n->right;
    }

    return n;
  }

  while ((parent = get_rb_parent(rb_node)) && rb_node == parent->left) {
    rb_node = parent;
  }

  return parent;
};

inline void rb_replace(RBNode *victim, RBNode *replacement, RBRoot *root) {
  RBNode *parent = get_rb_parent(victim);

  if (victim->left) {
    rb_set_parent(victim->left, replacement);
  }

  if (victim->right) {
    rb_set_parent(victim->right, replacement);
  }

  rb_change_child(victim, replacement, parent, root);
};

inline RBNode *rb_left_deepest_node(RBNode *rb_node) {
  while (true) {
    if (rb_node->left) {
      rb_node = rb_node->left;
    } else if (rb_node->right) {
      rb_node = rb_node->right;
    } else {
      return const_cast<RBNode *>(rb_node);
    }
  }
};

inline RBNode *rb_first_postorder(RBRoot *root) {
  if (!root->rb_root)
    return nullptr;
  return rb_left_deepest_node(root->rb_root);
};

inline RBNode *rb_next_postorder(const RBNode *rb_node) {
  if (!rb_node)
    return nullptr;

  RBNode *parent = get_rb_parent(rb_node);

  if (parent && rb_node == parent->left && parent->right) {
    return rb_left_deepest_node(parent->right);
  }

  return parent;
};
