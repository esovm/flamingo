#include <stdio.h>
#include <stdlib.h>

#include "list.h"

ListNode *list_node_new(void *val)
{
    ListNode *node = malloc(sizeof(ListNode));
    if (!node) return NULL;
    node->val = val;
    node->prev = NULL;
    node->next = NULL;
    return node;
}

ListIter *list_iter_new(List *list, ListDir dir)
{
    return list_iter_new_from_node(dir == LIST_HEAD ? list->head : list->tail, dir);
}

ListIter *list_iter_new_from_node(ListNode *node, ListDir dir)
{
    ListIter *it = malloc(sizeof(ListIter));
    if (!it) return NULL;
    it->next = node;
    it->dir = dir;
    return it;
}

ListNode *list_iter_next(ListIter *it)
{
    ListNode *curr = it->next;
    if (curr)
        it->next = it->dir == LIST_HEAD ? curr->next : curr->prev;
    return curr;
}

void list_iter_free(ListIter *it)
{
    free(it);
    it = NULL;
}

List *list_new(void)
{
    List *list = malloc(sizeof(List));
    if (!list) return NULL;
    list->head = NULL;
    list->tail = NULL;
    list->len = 0;
    return list;
}

ListNode *list_prepend(List *list, ListNode *node)
{
    if (!node) return NULL;

    if (list->len) {
        node->next = list->head;
        node->prev = NULL;
        list->head->prev = node;
        list->head = node;
    } else {
        list->head = list->tail = node;
        node->prev = node->next = NULL;
    }
    ++list->len;
    return node;
}

ListNode *list_append(List *list, ListNode *node)
{
    if (!node) return NULL;

    if (list->len) {
        node->prev = list->tail;
        node->next = NULL;
        list->tail->next = node;
        list->tail = node;
    } else {
        list->head = list->tail = node;
        node->prev = node->next = NULL;
    }
    ++list->len;
    return node;
}

ListNode *list_shift(List *list)
{
    if (!list->len) return NULL;

    ListNode *node = list->head;

    if (--list->len)
        (list->head = node->next)->prev = NULL;
    else
        list->head = list->tail = NULL;

    node->next = node->prev = NULL;
    return node;
}

ListNode *list_pop(List *list)
{
    if (!list->len) return NULL;

    ListNode *node = list->tail;

    if (--list->len)
        (list->tail = node->prev)->next = NULL;
    else
        list->tail = list->head = NULL;

    node->next = node->prev = NULL;
    return node;
}

ListNode *list_find(List *list, void *val)
{
    ListIter *it = list_iter_new(list, LIST_HEAD);
    ListNode *node;

    while ((node = list_iter_next(it)))
        if (node->val == val) {
            list_iter_free(it);
            return node;
        }

    list_iter_free(it);
    return NULL;
}

ListNode *list_at(List *list, int index)
{
    ListDir dir = LIST_HEAD;

    if (index < 0) {
        dir = LIST_TAIL;
        index = ~index;
    }
    if ((size_t)index < list->len) {
        ListIter *it = list_iter_new(list, dir);
        ListNode *node = list_iter_next(it);
        while (index--) node = list_iter_next(it);
        list_iter_free(it);
        return node;
    }

    return NULL;
}

void list_remove(List *list, ListNode *node)
{
    node->prev ? (node->prev->next = node->next) : (list->head = node->next);
    node->next ? (node->next->prev = node->prev) : (list->tail = node->prev);
    free(node);
    --list->len;
}

void list_free(List *list)
{
    size_t len = list->len;
    ListNode *next, *curr = list->head;

    while (len--) {
        next = curr->next;
        free(curr);
        curr = next;
    }

    free(list);
}
