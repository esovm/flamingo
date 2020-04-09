#ifndef LIST_H
#define LIST_H

#include <stddef.h>

#define list_dump(L, FS)                                          \
    do {                                                          \
        ListIter *it = list_iter_new(L, LIST_HEAD);               \
        ListNode *n;                                              \
        while ((n = list_iter_next(it)))                          \
            switch (FS) {                                         \
            case 'd': printf("%d -> ", *(int *)n->val); break;    \
            case 'f': printf("%f -> ", *(float *)n->val); break;  \
            case 'c': printf("%c -> ", *(char *)n->val); break;   \
            case 's': printf("%s -> ", (char *)n->val); break;    \
            default: printf("INVALID FORMAT SPECIFIER! -> ");     \
            }                                                     \
        puts("NULL");                                             \
        list_iter_free(it);                                       \
    } while (0)

typedef enum {
    LIST_HEAD,
    LIST_TAIL
} ListDir;

typedef struct ListNode {
    struct ListNode *prev;
    struct ListNode *next;
    void *val;
} ListNode;

typedef struct {
    ListNode *next;
    ListDir dir;
} ListIter;

typedef struct {
    ListNode *head;
    ListNode *tail;
    size_t len;
} List;

ListNode *list_node_new(void *);

ListIter *list_iter_new(List *, ListDir );
ListIter *list_iter_new_from_node(ListNode *, ListDir);
ListNode *list_iter_next(ListIter *);
void list_iter_free(ListIter *);

List *list_new(void);
ListNode *list_prepend(List *, ListNode *);
ListNode *list_append(List *, ListNode *);
ListNode *list_find(List *, void *);
ListNode *list_at(List *, int);
ListNode *list_shift(List *);
ListNode *list_pop(List *);
void list_remove(List *, ListNode *);
void list_free(List *);

#endif /* LIST_H */
