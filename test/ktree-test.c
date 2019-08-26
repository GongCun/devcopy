#include "error.h"
#include "ktree.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void print(int *data)
{
    printf("%d\n", *data);
}


int main(int argc, char *argv[])
{
    KTree *tree, *p;
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
    p = ktree_root(tree);
    
    ktree_ins_child(tree, NULL, &data);
    while (1) {
        data = malloc(sizeof(int));
        if (data == NULL) {
            err_sys("malloc data");
        }
        printf("Input child: ");
        scanf("%d", data);
        ktree_ins_child(tree, p, data);
        p = ktree_first_child(p);
    }
    ktree_print(tree, ktree_root(tree));
    ktree_destory(tree);
    
}
