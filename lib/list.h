#ifndef _LIST_H
#define _LIST_H

/* Define a structure for linked list elements. */
typedef struct ListElmt_ {
    void *data;
    struct ListElmt_ *next;
} ListElmt;

/* Define a structure for linked lists. */
typedef struct List_ {
    int size;
    int (*match) (const void *key1, const void *key2);
    void (*destroy) (void *data);
    ListElmt *head;
    ListElmt *tail;
} List;

/* Public Interface */
void list_init (List *list, void (*destroy) (void *data));
void list_destory(List *list);
int list_ins_next(List *list, ListElmt *element, const void *data);
int list_rem_next(List *list, ListElmt *element, void **data);
ListElmt *list_find(ListElmt *node, void *data);

#define list_size(list) ((list) -> size)
#define list_head(list) ((list) -> head)
#define list_tail(list) ((list) -> tail)
#define list_data(element) ((element) -> data)
#define list_next(element) ((element) -> next)

#endif

