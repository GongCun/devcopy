#ifndef _MY_KTREE_H
#define _MY_KTREE_H

#include "list.h"

/* Define the structure for k-ary tree nodes */
typedef struct KTreeNode_ {
    void *ktn_data;
    List *ktn_child;
} KTreeNode;

/* Define the struct for k-ary trees */
typedef struct KTree_ {
  int        kt_size;
  int        (*kt_compare)(const  void  *key1,  const  void  *key2);
  void       (*kt_destroy)(void   *data);
  KTreeNode  *kt_root;
} KTree;

/*-------------- Public Interface -------------- */
void ktree_init(KTree *tree, void (*destroy)(void *data));
void ktree_destory(KTree *tree);
int ktree_ins_child(KTree *tree, KTreeNode *node, const void *data);
void ktree_rem_child(KTree *tree, KTreeNode *node);

#define ktree_size(tree) ((tree) -> kt_size)
#define ktree_root(tree) ((tree) -> kt_root)
#define ktree_data(node) ((node) -> ktn_data)
/* Haven't define the child access function */

#endif


