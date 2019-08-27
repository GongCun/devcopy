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


int main(int argc, char *argv[])
{
    KTree *tree;
    KTreeNode *p;
    int *data;
    int i;
    
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

    /* printf("Find child: "); */
    /* scanf("%d", &i); */
    p = ktree_find(tree, ktree_root(tree), &i);
    if (p) {
        tree -> kt_print(p -> ktn_data);
        putchar('\n');
    }
    ktree_destroy(tree);
    
}
