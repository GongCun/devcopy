#include "error.h"
#include "ktree.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void print(void *data)
{
    printf("%d\n", *(int *)data);
}


int main(int argc, char *argv[])
{
    KTree *tree;
    KTreeNode *p;
    int *data;
    
    tree = malloc(sizeof(KTree));
    if (tree == NULL) {
        err_sys("malloc tree");
    }
    ktree_init(tree, free, print);

    data = malloc(sizeof(int));
    if (data == NULL) {
        err_sys("malloc data");
    }
    printf("Input root: ");
    scanf("%d", data);
    
    ktree_ins_child(tree, NULL, data);
    p = ktree_root(tree);

    while (1) {
        /* if (feof(stdin)) { */
        /*     break; */
        /* } */
        data = malloc(sizeof(int));
        if (data == NULL) {
            err_sys("malloc data");
        }
        printf("Input child: ");
        scanf("%d", data);
        if (*data == -1)
            break;
        ktree_ins_child(tree, p, data);
        p = ktree_first_child(p);
    }

    printf("size = %d\n", ktree_size(tree));
    ktree_print(tree, ktree_root(tree));
    ktree_destroy(tree);
    
}
