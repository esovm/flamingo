#include <stdlib.h>

#include "list.h"

Node *node_new(Object *data)
{
	Node *node = malloc(sizeof(Node));
	if (node) {
	    node->data = data;
	    node->next = NULL;
    }
	return node;
}

List *list_new(void)
{
    List *list = malloc(sizeof(List));
    if (list) {
        list->head = NULL;
        list->nelem = 0;
    }
    return list;
}

void list_free(List *list)
{
    if (!list) return;
}
