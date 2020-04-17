#ifndef FLAMINGO_ENV_H
#define FLAMINGO_ENV_H

#include "object.h"
#include "map.h"

typedef map_type(Object) map_object_T;

struct Env {
    map_object_T map;
    Env *parent;
};

Env *env_new(void);
Object *env_get(Env *, Object *);
void env_set(Env *, Object *, Object *);
void env_set_global(Env *, Object *, Object *);
Env *env_cp(Env *);
void env_register(Env *, const char *, BuiltinFn);
void env_register_all(Env *);
void env_free(Env *);

#endif /* FLAMINGO_ENV_H */
