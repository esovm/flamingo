#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "env.h"

Env *env_new(void)
{
    Env *ret = malloc(sizeof(Env));
    if (!ret) return NULL;

    ret->parent = NULL;
    map_init(&ret->map);

    return ret;
}

Object *env_get(Env *env, Object *obj)
{
    Object *findme;
    if ((findme = map_get(&env->map, obj->r.symbol)))
        return obj_cp(findme);

    return env->parent
        ? env_get(env->parent, obj)
        : obj_new_err("Use of undefined symbol '%s'", obj->r.symbol);
}

void env_set(Env *env, Object *key, Object *val)
{
    /* check for existing variable(s) */
    if (map_get(&env->map, key->r.symbol)) {
        map_remove(&env->map, key->r.symbol);
        map_set(&env->map, key->r.symbol, *obj_cp(val));
        return;
    }
    /* none found, insert new */
    map_set(&env->map, key->r.symbol, *obj_cp(val));
}

void env_set_global(Env *env, Object *key, Object *val)
{
    for (; env->parent; env = env->parent);
    env_set(env, key, val);
}

Env *env_cp(Env *env)
{
    Env *ret = malloc(sizeof(Env));
    if (!ret) return NULL;

    ret->parent = env->parent;
    ret->map = env->map;
    map_init(&ret->map);

    return ret;
}

void env_register(Env *env, const char *id, BuiltinFn func)
{
    Object *key = obj_new_sym(id);
    Object *obj = obj_new_func(func);
    env_set(env, key, obj);
    obj_free(key);
    obj_free(obj);
}

void env_register_all(Env *env)
{
    env_register(env, "exit", bi_exit);

    env_register(env, "list", bi_list);
    env_register(env, "first", bi_first);
    env_register(env, "last", bi_last);
    env_register(env, "rest", bi_rest);
    env_register(env, "eval", bi_eval);
    env_register(env, "attach", bi_attach);
    env_register(env, "init", bi_init);

    env_register(env, "+", bi_add);
    env_register(env, "-", bi_sub);
    env_register(env, "*", bi_mul);
    env_register(env, "/", bi_div);
    env_register(env, "//", bi_idiv);
    env_register(env, "%", bi_mod);
    env_register(env, "^", bi_bxor);
    env_register(env, "&", bi_band);
    env_register(env, "|", bi_bor);
    env_register(env, "~", bi_bnot);
    env_register(env, "min", bi_min);
    env_register(env, "max", bi_max);

    env_register(env, "<", bi_lt);
    env_register(env, "<=", bi_le);
    env_register(env, ">", bi_gt);
    env_register(env, ">=", bi_ge);
    env_register(env, "==", bi_eq);
    env_register(env, "!=", bi_ne);
    env_register(env, "if", bi_if);

    env_register(env, "not", bi_not);
    env_register(env, "or", bi_or);
    env_register(env, "and", bi_and);

    env_register(env, "def", bi_def);
    env_register(env, "=", bi_loc);
    env_register(env, "$", bi_lambda);

    env_register(env, "use", bi_use);
    env_register(env, "puts", bi_puts);
    env_register(env, "err", bi_err);
    env_register(env, "while", bi_while);
    
}

void env_free(Env *env)
{
    map_free(&env->map);
    free(env);
}
