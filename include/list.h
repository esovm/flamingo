#ifndef LIST_H
#define LIST_H

#include "object.h"

typedef struct Node {
    Object *data;
    struct Node *next;
} Node;

// List *list_new(void);

#endif
