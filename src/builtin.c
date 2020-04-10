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
    NARG("first", obj, 1);
    EXPECT("first", obj, 0, O_BEXPR);
    OBJ_ENSURE(obj, obj->cell[0]->nelem, "first cannot operate on empty b-expression ('[]')");

    Object *first = obj_take(obj, 0);
    while (first->nelem > 1)
        obj_free(obj_pop(first, 1));
    return first;
}

/* return a b-expression without its first element */
Object *bi_rest(Env *env, Object *obj)
{
    NARG("rest", obj, 1);
    EXPECT("rest", obj, 0, O_BEXPR);
    OBJ_ENSURE(obj, obj->cell[0]->nelem, "rest cannot operate on empty b-expression ('[]')");

    Object *first = obj_take(obj, 0);
    obj_free(obj_pop(first, 0));
    return first;
}

Object *bi_list(Env *env, Object *obj)
{
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
    NARG("init", obj, 1);
    EXPECT("init", obj, 0, O_BEXPR);
    OBJ_ENSURE(obj, obj->cell[0]->nelem, "init cannot operate on empty b-expression ('[]')");

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
        else if (EQ("def", func))
            env_set_global(env, symbols->cell[i], list->cell[i + 1]);
    }

    obj_free(list);

    return obj_new_sexpr();
}

Object *bi_lambda(Env *env, Object *list)
{
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
    EXPECT("if", list, 0, O_NUMBER);
    EXPECT("if", list, 1, O_BEXPR);
    EXPECT("if", list, 2, O_BEXPR);

    list->cell[1]->type = list->cell[2]->type = O_SEXPR;

    Object *ret = list->cell[0]->r.number
        ? obj_eval(env, obj_pop(list, 1))
        : obj_eval(env, obj_pop(list, 2));

    obj_free(list);
    return ret;
}

Object *bi_use(Env *env, Object *list)
{
    NARG("use", list, 1);
    EXPECT("use", list, 0, O_STRING);

    mpc_result_T res;
    if (mpc_parse_contents(list->cell[0]->r.string, gg, &res)) {
        Object *expression = obj_read(res.output);
        mpc_ast_delete(res.output);

        while (expression->nelem) {
            Object *a;
            if ((a = obj_eval(env, obj_pop(expression, 0)))->type == O_ERROR) {
                obj_dump(a);
                putchar('\n');
            }
            obj_free(a);
        }
        obj_free(expression);
        obj_free(list);

        return obj_new_sexpr();
    }

    char *msg = mpc_err_string(res.error);
    mpc_err_delete(res.error);
    Object *error = obj_new_err("Cannot use file %s", msg);
    free(msg);
    obj_free(list);
    return error;
}

Object *bi_show(Env *env, Object *list)
{
    for (size_t i = 0; i < list->nelem; ++i) {
        obj_dump(list->cell[i]);
        putchar(' ');
    }
    putchar('\n');
    obj_free(list);
    return obj_new_sexpr();
}
