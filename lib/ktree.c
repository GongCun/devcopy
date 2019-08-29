#include "ktree.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


void ktree_init(KTree *tree,
                void (*destroy)(void *data),
                void (*print)(void *data))
{
    tree -> kt_size = 0;
    tree -> kt_destroy = destroy;
    tree -> kt_root = NULL;
    tree -> kt_print = print;
}


int ktree_ins_child(KTree *tree, KTreeNode *node, const void *data) {
    KTreeNode *new_node;

    /* Allocate storage for new node */
    new_node = (KTreeNode *)malloc(sizeof(KTreeNode));
    if (new_node == NULL) {
        return -1;
    }

    ktree_data(new_node) = (void *)data;
    ktree_first_child(new_node) = NULL;

    /* Insert a node into the tree as the child of a specific node */
    if (node == NULL) {
        /* Insert at the root of the tree */
        ktree_parent(new_node) = NULL;
        ktree_next_sibling(new_node) = NULL;
        ktree_first_child(new_node) = ktree_root(tree);
        ktree_root(tree) = new_node;
    }
    else {
        ktree_parent(new_node) = node;
        ktree_next_sibling(new_node) = ktree_first_child(node);
        ktree_first_child(node) = new_node;
    }
    tree -> kt_size++;
    return 0;
}

void ktree_rem_first(KTree *tree, KTreeNode *node)
{
    KTreeNode *position, *p;

    if (ktree_size(tree) == 0)
        return;

    if (node == NULL) {
        position = tree->kt_root;
    }
    else {
        position = node->ktn_first_child;
    }

    if (position) {

        if (node) {
            node -> ktn_first_child = position -> ktn_next_sibling;
        }

        p = position -> ktn_first_child;
        while (p && p -> ktn_next_sibling)
            ktree_rem_next(tree, p);

        ktree_rem_first(tree, position);

        if (tree -> kt_destroy) {
            tree -> kt_destroy(position -> ktn_data);
        }

        free(position);
        position = NULL;
        tree -> kt_size--;
    }

}

void ktree_rem_next(KTree *tree, KTreeNode *node)
{
    KTreeNode *position, *p;

    if (ktree_size(tree) == 0 ||
        node == NULL)
        {
            return;
        }

    position = node -> ktn_next_sibling;
    
    if (position) {
        node -> ktn_next_sibling = position -> ktn_next_sibling;

        p = position;
        while (p && p->ktn_first_child)
            ktree_rem_first(tree, p);

        if (tree->kt_destroy) {
            tree->kt_destroy(position -> ktn_data);
        }

        free(position);
        position = NULL;
        tree->kt_size--;
    }

}

void ktree_destroy(KTree *tree)
{
    ktree_rem_first(tree, NULL);
}

void ktree_print(KTree *tree, KTreeNode *node)
{
    KTreeNode *p;
    
    if (node == NULL)
        return;
    /* Print current node. */
    if (tree->kt_print) {
        tree -> kt_print (node -> ktn_data);
    }

    /* Print child nodes. */
    for (p = node -> ktn_first_child;
         p;
         p = p -> ktn_next_sibling)
        {
            ktree_print(tree, p);
        }
}

KTreeNode *ktree_find(KTree *tree, KTreeNode *node, void *data)
{
    KTreeNode *ret, *p;

    if (tree -> kt_compare == NULL)
        return NULL;

    if (node == NULL)
        return NULL;

    if (tree -> kt_compare(node -> ktn_data, data)) {
        return node;
    }
    else {
        for (p = node -> ktn_first_child;
             p;
             p = p ->ktn_next_sibling)
            {
                ret = ktree_find(tree, p, data);
                if (ret)
                    return ret;
            }
    }
    return NULL;
}


int ktree_path(KTree *tree, KTreeNode *node1, KTreeNode *node2, KTreeNode *xnode, List *list)
{
    KTreeNode *p, *parent, *tmp;
    ListElmt *t, *t2;
    
    t = list -> tail;
    list_ins_next(list, t, (const void *)node1);
    
    if (node1 == node2)
        return 1;

    /* Path: node1 -> child -> node2 */
    for (p = node1 -> ktn_first_child;
         p;
         p = p -> ktn_next_sibling)
        {
            /* t2 = list -> tail; */
            /* list_ins_next(list, t2, (const void *)p); */
            if (p == xnode)
                continue;

            if (ktree_path(tree, p, node2, xnode, list)) {
                return 1;
            }

            /* list_rem_next(list, t2, (void **)&tmp); */
        }

    p = node1 -> ktn_parent;
    if (ktree_path(tree, p, node2, node1, list))
        return 1;
    
    

    /* /\* Path: node1 -> parent -> ... -> node2 *\/ */
    /* for (parent = node1 -> ktn_parent; */
    /*      parent; */
    /*      parent = parent -> ktn_parent) */
    /*     { */
    /*         /\* t = list -> tail; *\/ */
    /*         /\* list_ins_next(list, t, (const void *)parent); *\/ */


    /*         if (parent == node2) */
    /*             return 1; */
    
    /*         for (p = parent -> ktn_first_child; */
    /*              p; */
    /*              p = p -> ktn_next_sibling) */
    /*             { */
    /*                 if (p == node1) */
    /*                     continue; */

    /*                 t2 = list -> tail; */
    /*                 list_ins_next(list, t2, (const void *)p); */

    /*                 if (ktree_path(tree, p, node2, list)) { */
    /*                     return 1; */
    /*                 } */

    /*                 list_rem_next(list, t2, (void **)&tmp); */
    /*             } */

    /*         /\* list_rem_next(list, t, (void **)&tmp); *\/ */
    /*     } */

    list_rem_next(list, t, (void **)&tmp);
    return 0;
    
}
