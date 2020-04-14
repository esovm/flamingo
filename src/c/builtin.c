#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "object.h"
#include "env.h"
#include "util.h"

/* bi_ prefixed functions - built-ins */

Object *bi_exit(Env *env, Object *obj)
{
    int status;

    NARG("exit", obj, 1);
    EXPECT("exit", obj, 0, O_NUMBER);

    status = obj->cell[0]->r.number;

    obj_free(obj);
    env_free(env);

    exit(status);
}

/* return the first element of a b-expression */
Object *bi_first(Env *env, Object *obj)
{
    (void)env;
    NARG("first", obj, 1);
    EXPECT("first", obj, 0, O_BEXPR);
    NOT_EMPTY("first", obj, 0);

    Object *first = obj_take(obj, 0);
    while (first->nelem > 1)
        obj_free(obj_pop(first, 1));
    return first;
}

/* return the last element of a b-expression */
Object *bi_last(Env *env, Object *obj)
{
    (void)env;
    NARG("last", obj, 1);
    EXPECT("last", obj, 0, O_BEXPR);
    NOT_EMPTY("last", obj, 0);

    Object *last = obj_take(obj, obj->nelem - 1);
    while (last->nelem > 1)
        obj_free(obj_pop(last, 0));
    return last;
}

/* return a b-expression without its first element */
Object *bi_rest(Env *env, Object *obj)
{
    (void)env;
    NARG("rest", obj, 1);
    EXPECT("rest", obj, 0, O_BEXPR);
    NOT_EMPTY("rest", obj, 0);

    Object *first = obj_take(obj, 0);
    obj_free(obj_pop(first, 0));
    return first;
}

Object *bi_list(Env *env, Object *obj)
{
    (void)env;
    obj->type = O_BEXPR;
    return obj;
}

Object *bi_eval(Env *env, Object *obj)
{
    NARG("eval", obj, 1);
    EXPECT("eval", obj, 0, O_BEXPR);

    Object *ret = obj_take(obj, 0);
    ret->type = O_SEXPR;
    return obj_eval(env, ret);
}

Object *bi_attach(Env *env, Object *obj)
{
    (void)env;
    for (size_t i = 0; i < obj->nelem; ++i)
        EXPECT("attach", obj, i, O_BEXPR);

    Object *ret = obj_pop(obj, 0);
    while (obj->nelem)
        ret = obj_attach(ret, obj_pop(obj, 0));

    obj_free(obj);
    return ret;
}

/* return b-expression without the last element */
Object *bi_init(Env *env, Object *obj)
{
    (void)env;
    NARG("init", obj, 1);
    EXPECT("init", obj, 0, O_BEXPR);
    NOT_EMPTY("init", obj, 0);

    Object *first = obj_take(obj, 0);
    obj_free(obj_pop(first, first->nelem - 1));
    return first;
}

Object *bi_var(Env *env, Object *list, const char *func)
{
    EXPECT(func, list, 0, O_BEXPR);

    Object *symbols = list->cell[0];
    for (size_t i = 0; i < symbols->nelem; ++i)
        OBJ_ENSURE_F(list, symbols->cell[i]->type == O_SYMBOL, "can only define symbol (got %s)",
            obj_type_arr[symbols->cell[i]->type]);

    OBJ_ENSURE_F(list, symbols->nelem == list->nelem - 1,
        "incorrect number of arguments (%d) for %s", list->nelem - 1, func);

    for (size_t i = 0; i < symbols->nelem; ++i) {
        if (*func == '=')
            env_set(env, symbols->cell[i], list->cell[i + 1]);
        else if (!strcmp("def", func))
            env_set_global(env, symbols->cell[i], list->cell[i + 1]);
    }

    obj_free(list);

    return obj_new_sexpr();
}

Object *bi_lambda(Env *env, Object *list)
{
    (void)env;
    NARG("$", list, 2);
    EXPECT("$", list, 0, O_BEXPR);
    EXPECT("$", list, 1, O_BEXPR);

    for (size_t i = 0; i < list->cell[0]->nelem; ++i)
        OBJ_ENSURE_F(list, list->cell[0]->cell[i]->type == O_SYMBOL,
            "can only define symbol. (got %s)", obj_type_arr[list->cell[i]->type]);

    Object *params = obj_pop(list, 0);
    Object *body = obj_pop(list, 0);

    obj_free(list);

    return obj_new_lambda(params, body);
}

Object *bi_if(Env *env, Object *list)
{
    NARG("if", list, 3);
    EXPECT("if", list, 1, O_BEXPR);
    EXPECT("if", list, 2, O_BEXPR);

    list->cell[1]->type = list->cell[2]->type = O_SEXPR;

    Object *ret = obj_is_truthy(list->cell[0])
        ? obj_eval(env, obj_pop(list, 1))
        : obj_eval(env, obj_pop(list, 2));

    obj_free(list);
    return ret;
}

Object *bi_use(Env *env, Object *list)
{
    NARG("use", list, 1);
    EXPECT("use", list, 0, O_STRING);

    char *name = list->cell[0]->r.string, *data;
    bool to_free = false;

    if (!!strcmp(&list->cell[0]->r.string[strlen(list->cell[0]->r.string) - 3], ".fl")) {
        /* + 4 for '.fl' extension and null terminator */
        name = malloc(strlen(list->cell[0]->r.string) + 4);
        strcpy(name, list->cell[0]->r.string);
        strcat(name, ".fl");
        to_free = true;
    }
    if (!(data = readfile(name))) {
        Object *error = obj_new_err("Cannot use file \"%s\"", name);
        obj_free(list);
        return error;
    }

    size_t pos = 0;
    Object *expr = obj_read_expr(data, &pos, '\0');
    free(data);

    if (expr->type != O_ERROR) {
        while (expr->nelem) {
            Object *a;
            if ((a = obj_eval(env, obj_pop(expr, 0)))->type == O_ERROR)
                OBJ_DUMP_LN(a);
            obj_free(a);
        }
    } else {
        OBJ_DUMP_LN(expr);
    }

    if (to_free) free(name);
    obj_free(expr);
    obj_free(list);

    return obj_new_sexpr();
}

Object *bi_puts(Env *env, Object *list)
{
    (void)env;
    for (size_t i = 0; i < list->nelem; ++i) {
        obj_dump(list->cell[i]);
        putchar(' ');
    }
    putchar('\n');
    obj_free(list);
    return obj_new_sexpr();
}

Object *bi_err(Env *env, Object *list)
{
    (void)env;
    NARG("err", list, 1);
    EXPECT("err", list, 0, O_STRING);

    Object *ret = obj_new_err(list->cell[0]->r.string);
    obj_free(list);
    return ret;
}
