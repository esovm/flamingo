#ifndef LIST_H
#define LIST_H

#include "object.h"

typedef struct Node {
    Object *data;
    struct Node *next;
} Node;

typedef struct List {
    Node *head;
    size_t nelem;
} List;

Node *node_new(Object *data);
List *list_new(void);
void list_free(List *list);

#endif
