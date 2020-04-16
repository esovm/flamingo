#ifndef MAP_H
#define MAP_H

#include <string.h>

typedef struct MapNode MapNode;

typedef struct {
    MapNode **buckets;
    unsigned nbuckets, nnodes;
} MapBase;

typedef struct {
    unsigned bucketidx;
    MapNode *node;
} MapIter;

#define map_t(T)      \
    struct {          \
        MapBase base; \
        T *ref;       \
        T tmp;        \
    }

#define map_init(m) memset(m, 0, sizeof(*(m)))

#define map_free(m) map_free_(&(m)->base)

#define map_get(m, key) ((m)->ref = map_get_(&(m)->base, key))

#define map_set(m, key, value) \
    ((m)->tmp = (value),       \
    map_set_(&(m)->base, key, &(m)->tmp, sizeof((m)->tmp)))

#define map_remove(m, key) \
    map_remove_(&(m)->base, key)

#define map_iter(m) \
    map_iter_()

#define map_next(m, iter) \
    map_next_(&(m)->base, iter)

void map_free_(MapBase *m);
void *map_get_(MapBase *m, const char *key);
int map_set_(MapBase *m, const char *key, void *value, int vsize);
void map_remove_(MapBase *m, const char *key);
MapIter map_iter_(void);
const char *map_next_(MapBase *m, MapIter *iter);
typedef map_t(void *) map_void_t;
typedef map_t(char *) map_str_t;
typedef map_t(int) map_int_t;
typedef map_t(char) map_char_t;
typedef map_t(float) map_float_t;
typedef map_t(double) map_double_t;

#endif /* MAP_H */
