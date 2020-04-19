#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "object.h"
#include "env.h"
#include "util.h"

/* bi_ prefixed functions - built-ins */

Object *bi_exit(Env *env, Object *obj)
{
    NARG("exit", obj, 1);
    EXPECT("exit", obj, 0, O_NUMBER);

    int status = obj->cell[0]->r.real;
    obj_free(obj);
    env_free(env);
    exit(status);
}

/* return the first element of a b-expression, or the first character of a string */
Object *bi_first(Env *env, Object *obj)
{
    UNUSED(env);
    NARG("first", obj, 1);

    if (obj->cell[0]->type == O_BEXPR) {
        BEXPR_NOT_EMPTY("first", obj, 0);
        Object *first = obj_take(obj, 0);
        while (first->nelem > 1)
            obj_free(obj_pop(first, 1));
        return first;
    } else if (obj->cell[0]->type == O_STRING) {
        STRING_NOT_EMPTY("first", obj, 0);
        char first = *obj->cell[0]->r.string;
        obj_free(obj);
        return obj_new_str(&first);
    }
    const char *got = obj_type_arr[obj->cell[0]->type];
    obj_free(obj);
    return obj_new_err("first expected a b-expression or a string but got %s", got);
}

/* return the last element of a b-expression, or the last character of a string */
Object *bi_last(Env *env, Object *obj)
{
    UNUSED(env);
    NARG("last", obj, 1);

    if (obj->cell[0]->type == O_BEXPR) {
        BEXPR_NOT_EMPTY("last", obj, 0);
        Object *last = obj_take(obj, obj->nelem - 1);
        while (last->nelem > 1)
            obj_free(obj_pop(last, 0));
        return last;
    } else if (obj->cell[0]->type == O_STRING) {
        STRING_NOT_EMPTY("last", obj, 0);
        char last = obj->cell[0]->r.string[strlen(obj->cell[0]->r.string) - 1];
        obj_free(obj);
        return obj_new_str(&last);
    }
    const char *got = obj_type_arr[obj->cell[0]->type];
    obj_free(obj);
    return obj_new_err("last expected a b-expression or a string but got %s", got);
}

/* return a b-expression without its first element */
Object *bi_rest(Env *env, Object *obj)
{
    UNUSED(env);
    NARG("rest", obj, 1);

    if (obj->cell[0]->type == O_BEXPR) {
        BEXPR_NOT_EMPTY("rest", obj, 0);
        Object *first = obj_take(obj, 0);
        obj_free(obj_pop(first, 0));
        return first;
    } else if (obj->cell[0]->type == O_STRING) {
        STRING_NOT_EMPTY("rest", obj, 0);
        size_t len = strlen(obj->cell[0]->r.string);
        char cp[len];
        for (int i = 0; obj->cell[0]->r.string[i]; ++i)
            cp[i] = obj->cell[0]->r.string[i];
        memmove(cp, cp + 1, len);
        obj_free(obj);
        return obj_new_str(cp);
    }
    const char *got = obj_type_arr[obj->cell[0]->type];
    obj_free(obj);
    return obj_new_err("rest expected a b-expression or a string but got %s", got);
}

Object *bi_list(Env *env, Object *obj)
{
    UNUSED(env);
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
    UNUSED(env);
    for (int i = 0; i < obj->nelem; ++i)
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
    UNUSED(env);
    NARG("init", obj, 1);
    EXPECT("init", obj, 0, O_BEXPR);
    BEXPR_NOT_EMPTY("init", obj, 0);

    Object *first = obj_take(obj, 0);
    obj_free(obj_pop(first, first->nelem - 1));
    return first;
}

Object *bi_lambda(Env *env, Object *list)
{
    UNUSED(env);
    NARG("$", list, 2);
    EXPECT("$", list, 0, O_BEXPR);
    EXPECT("$", list, 1, O_BEXPR);

    for (int i = 0; i < list->cell[0]->nelem; ++i)
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

Object *bi_use(Env *env, Object *obj)
{
    NARG("use", obj, 1);
    EXPECT("use", obj, 0, O_STRING);

    char *data = readfile(obj->cell[0]->r.string);
    if (!data) {
        Object *error = obj_new_err("Cannot use file \"%s\"", obj->cell[0]->r.string);
        obj_free(obj);
        return error;
    }
    int pos = 0;
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

    obj_free(expr);
    obj_free(obj);
    return obj_new_sexpr();
}

Object *bi_puts(Env *env, Object *list)
{
    UNUSED(env);
    for (int i = 0; i < list->nelem; ++i) {
        obj_dump(list->cell[i]);
        putchar(' ');
    }
    putchar('\n');
    obj_free(list);
    return obj_new_sexpr();
}

Object *bi_err(Env *env, Object *list)
{
    UNUSED(env);
    NARG("err", list, 1);
    EXPECT("err", list, 0, O_STRING);

    Object *ret = obj_new_err(list->cell[0]->r.string);
    obj_free(list);
    return ret;
}

/**
 * NOT GOOD: Evaluating expression frees it,
 * having to use obj_cp which uses malloc every iteration, which is slow
 */
Object *bi_while(Env *env, Object *list)
{
    NARG("while", list, 2);
    EXPECT("while", list, 0, O_BEXPR);
    EXPECT("while", list, 1, O_BEXPR);

    list->cell[0]->type = list->cell[1]->type = O_SEXPR;

    Object *cond = obj_eval(env, obj_cp(list->cell[0]));

    while (obj_is_truthy(cond)) {
        obj_free(obj_eval(env, obj_cp(list->cell[1])));
        obj_free(cond);
        cond = obj_eval(env, obj_cp(list->cell[0]));
    }

    obj_free(cond);
    obj_free(list);
    return obj_new_sexpr();
}
