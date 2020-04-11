#ifndef FLAMINGO_ENV_H
#define FLAMINGO_ENV_H

#include "object.h"

struct Env {
    size_t nelem;
    char **sym_list;
    Object **obj_list;
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
