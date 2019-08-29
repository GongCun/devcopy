#ifndef _MY_KTREE_H
#define _MY_KTREE_H

#include "list.h"

/* Define the structure for k-ary tree nodes */
typedef struct KTreeNode_ {
    void              *ktn_data;
    struct KTreeNode_ *ktn_parent;
    struct KTreeNode_ *ktn_first_child;
    struct KTreeNode_ *ktn_next_sibling;
} KTreeNode;

/* Define the struct for k-ary trees */
typedef struct KTree_ {
  int        kt_size;
  int        (*kt_compare)(const void *key1,  const void *key2);
  void       (*kt_destroy)(void *data);
  void       (*kt_print)(void *data);
  KTreeNode  *kt_root;
} KTree;

/*-------------- Public Interface -------------- */
void ktree_init(KTree *tree, void (*destroy)(void *data), void (*print)(void *data));
void ktree_destroy(KTree *tree);
int ktree_ins_child(KTree *tree, KTreeNode *node, const void *data);
void ktree_rem_first(KTree *tree, KTreeNode *node);
void ktree_rem_next(KTree *tree, KTreeNode *node);
void ktree_print(KTree *tree, KTreeNode *node);
KTreeNode *ktree_find(KTree *tree, KTreeNode *node, void *data);
int ktree_path(KTree *tree, KTreeNode *node1, KTreeNode *node2, KTreeNode *xnode, List *list);

#define ktree_size(tree) ((tree) -> kt_size)
#define ktree_root(tree) ((tree) -> kt_root)
#define ktree_data(node) ((node) -> ktn_data)
#define ktree_parent(node) ((node) -> ktn_parent)
#define ktree_first_child(node) ((node) -> ktn_first_child)
#define ktree_next_sibling(node) ((node) -> ktn_next_sibling)
#endif


