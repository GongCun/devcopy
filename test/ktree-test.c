#include "error.h"
#include "ktree.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void print(void *data)
{
    printf("%d ", *(int *)data);
}

static int compare(const void *key1, const void *key2)
{
    return *(int *)key1 == *(int *)key2 ? 1 : 0;
}

static void compress_path(List *list)
{
    ListElmt *p;
    KTreeNode *t;

    p = list -> head;
    while (p) {
        while (list_find(p -> next, p -> data)) {
            while (1) {
                list_rem_next(list, p, (void **)&t);
                if (t == p -> data)
                    break;
            }
        }
        p = p -> next;
    }
}


int main(int argc, char *argv[])
{
    KTree *tree;
    KTreeNode *p, *n1, *n2;
    int *data;
    int i, j;
    List *list, *miss;
    
    tree = malloc(sizeof(KTree));
    if (tree == NULL) {
        err_sys("malloc tree");
    }
    ktree_init(tree, free, print);
    tree -> kt_compare = compare;

    data = malloc(sizeof(int));
    if (data == NULL) {
        err_sys("malloc data");
    }
    printf("Input root: ");
    scanf("%d", data);
    
    ktree_ins_child(tree, NULL, data);
    p = ktree_root(tree);

    while (1) {
        ktree_print(tree, ktree_root(tree));
        putchar('\n');
        do {
            printf("Insert child of: ");
            scanf("%d", &i);
            p = ktree_find(tree, ktree_root(tree), &i);
        } while (p == NULL);

        data = malloc(sizeof(int));
        if (data == NULL) {
            err_sys("malloc data");
        }
        printf("Input child: ");
        scanf("%d", data);
        if (*data == -1)
            break;

        ktree_ins_child(tree, p, data);
    }

    printf("size = %d\n", ktree_size(tree));
    ktree_print(tree, ktree_root(tree));
    putchar('\n');

    /* test find path */
    printf("input start and stop node: ");
    scanf("%d %d", &i, &j);
    printf("i = %d, j = %d\n", i, j);

    list = malloc(sizeof(List));
    if (list == NULL) {
        err_sys("malloc list");
    }
    list_init(list, free);

    miss = malloc(sizeof(List));
    if (miss == NULL) {
        err_sys("malloc list");
    }
    list_init(miss, free);

    n1 = ktree_find(tree, ktree_root(tree), &i);
    n2 = ktree_find(tree, ktree_root(tree), &j);
    int ret = ktree_path(tree, n1, n2, miss, list);
    printf("ret = %d\n", ret);

    ktree_print2d(tree, tree->kt_root, " ");

    compress_path(list);

    ListElmt *save = NULL;
    for (ListElmt *e = list -> head;
         e;
         e = e -> next)
        {
            p = (KTreeNode *)e -> data;
            if (save) {
                KTreeNode *tmp = (KTreeNode *)save -> data;
                printf("%s ", tmp -> ktn_parent == p ? "up": "down");
            }
            save = e;
            tree -> kt_print (p -> ktn_data);
        }
    putchar('\n');

    ktree_destroy(tree);
    printf("size: %d\n", tree -> kt_size);
    
}
