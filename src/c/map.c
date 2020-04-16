#include <stdlib.h>
#include <string.h>

#include "map.h"

struct MapNode {
    unsigned hash;
    void *value;
    MapNode *next;
};

static unsigned map_hash(const char *str)
{
    unsigned hash = 5381;
    while (*str)
        hash = ((hash << 5) + hash) ^ *str++;
    return hash;
}

static MapNode *map_newnode(const char *key, void *value, int vsize)
{
    MapNode *node;
    int ksize = strlen(key) + 1;
    int voffset = ksize + ((sizeof(void *) - ksize) % sizeof(void *));
    node = malloc(sizeof(*node) + voffset + vsize);
    if (!node) return NULL;
    memcpy(node + 1, key, ksize);
    node->hash = map_hash(key);
    node->value = ((char *)(node + 1)) + voffset;
    memcpy(node->value, value, vsize);
    return node;
}

static int map_bucketidx(MapBase *m, unsigned hash)
{
    /* If the implementation is changed to allow a non-power-of-2 bucket count,
    * the line below should be changed to use mod instead of AND */
    return hash & (m->nbuckets - 1);
}

static void map_addnode(MapBase *m, MapNode *node)
{
    int n = map_bucketidx(m, node->hash);
    node->next = m->buckets[n];
    m->buckets[n] = node;
}

static int map_resize(MapBase *m, int nbuckets)
{
    MapNode *node, *next;
    MapNode **buckets;
    /* Chain all nodes together */
    MapNode *nodes = NULL;
    int i = m->nbuckets;
    while (i--) {
        node = (m->buckets)[i];
        while (node) {
            next = node->next;
            node->next = nodes;
            nodes = node;
            node = next;
        }
    }
    /* Reset buckets */
    if ((buckets = realloc(m->buckets, sizeof(*m->buckets) * nbuckets))) {
        m->buckets = buckets;
        m->nbuckets = nbuckets;
    }
    if (m->buckets) {
        memset(m->buckets, 0, sizeof(*m->buckets) * m->nbuckets);
        /* Re-add nodes to buckets */
        node = nodes;
        while (node) {
            next = node->next;
            map_addnode(m, node);
            node = next;
        }
    }
    /* Return error code if realloc() failed */
    return buckets == NULL ? -1 : 0;
}

static MapNode **map_getref(MapBase *m, const char *key)
{
    unsigned hash = map_hash(key);
    MapNode **next;
    if (m->nbuckets > 0) {
        next = &m->buckets[map_bucketidx(m, hash)];
        while (*next) {
            if ((*next)->hash == hash && strcmp((char *)(*next + 1), key) == 0)
                return next;
            next = &(*next)->next;
        }
    }
    return NULL;
}

void map_free_(MapBase *m)
{
    MapNode *next, *node;
    int i = m->nbuckets;
    while (i--) {
        node = m->buckets[i];
        while (node) {
            next = node->next;
            free(node);
            node = next;
        }
    }
    free(m->buckets);
}

void *map_get_(MapBase *m, const char *key)
{
    MapNode **next = map_getref(m, key);
    return next ? (*next)->value : NULL;
}

int map_set_(MapBase *m, const char *key, void *value, int vsize)
{
    int n, err;
    MapNode **next, *node;
    /* Find & replace existing node */
    if ((next = map_getref(m, key))) {
        memcpy((*next)->value, value, vsize);
        return 0;
    }
    /* Add new node */
    if ((node = map_newnode(key, value, vsize)) == NULL) goto fail;
    if (m->nnodes >= m->nbuckets) {
        n = (m->nbuckets > 0) ? (m->nbuckets << 1) : 1;
        err = map_resize(m, n);
        if (err) goto fail;
    }
    map_addnode(m, node);
    ++m->nnodes;
    return 0;
fail:
    free(node);
    return -1;
}

void map_remove_(MapBase *m, const char *key)
{
    MapNode *node;
    MapNode **next = map_getref(m, key);
    if (next) {
        node = *next;
        *next = (*next)->next;
        free(node);
        --m->nnodes;
    }
}

MapIter map_iter_(void)
{
    MapIter iter;
    iter.bucketidx = -1;
    iter.node = NULL;
    return iter;
}

const char *map_next_(MapBase *m, MapIter *iter)
{
    if (iter->node) {
        iter->node = iter->node->next;
        if (iter->node == NULL) goto nextBucket;
    } else {
    nextBucket:
        do {
            if (++iter->bucketidx >= m->nbuckets) return NULL;
            iter->node = m->buckets[iter->bucketidx];
        } while (iter->node == NULL);
    }
    return (char *)(iter->node + 1);
}
