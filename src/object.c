#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "mpc.h"
#include "util.h"
#include "object.h"
#include "env.h"

/* these strings have to exactly match the `obj_type` enum elements */
static const char *const obj_type_arr[] = {
    "number", "error", "symbol", "function", "s-expression", "b-expression"
};

Object *obj_new_num(double n)
{
    Object *ret = s_malloc(sizeof(Object));

    ret->type = O_NUMBER;
    ret->r.number = n;

    return ret;
}

Object *obj_new_err(const char *fmt, ...)
{
    Object *ret = s_malloc(sizeof(Object));
    const size_t amt = 256;
    va_list ap;

    va_start(ap, fmt);

    ret->type = O_ERROR;
    ret->r.error = s_malloc(amt);
    vsnprintf(ret->r.error, amt - 1, fmt, ap);
    ret->r.error = s_realloc(ret->r.error, strlen(ret->r.error) + 1);

    va_end(ap);

    return ret;
}

Object *obj_new_sym(const char *s)
{
    Object *ret = s_malloc(sizeof(Object));

    ret->type = O_SYMBOL;
    ret->r.symbol = dupstr(s);

    return ret;
}

Object *obj_new_func(BuiltinFn func)
{
    Object *ret = s_malloc(sizeof(Object));

    ret->type = O_FUNC;
    ret->r.f.builtin = func;

    return ret;
}

Object *obj_new_lambda(Object *params, Object *body)
{
    Object *ret = s_malloc(sizeof(Object));

    ret->type = O_FUNC;
    ret->r.f.builtin = NULL;
    ret->r.f.params = params;
    ret->r.f.body = body;
    ret->r.f.env = env_new();

    return ret;
}

Object *obj_new_sexpr(void)
{
    Object *ret = s_malloc(sizeof(Object));

    ret->type = O_SEXPR;
    ret->nelem = 0;
    ret->cell = NULL;

    return ret;
}

Object *obj_new_bexpr(void)
{
    Object *ret = s_malloc(sizeof(Object));

    ret->type = O_BEXPR;
    ret->nelem = 0;
    ret->cell = NULL;

    return ret;
}

/* bi_ prefixed functions - bulit-ins */

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

Object *bi_car(Env *env, Object *obj)
{
    NARG("car", obj, 1);
    EXPECT("car", obj, 0, O_BEXPR);
    OBJ_ENSURE(obj, obj->cell[0]->nelem, "car cannot operate on empty b-expression ('[]')");

    Object *first = obj_take(obj, 0);
    while (first->nelem > 1)
        obj_free(obj_pop(first, 1));
    return first;
}

Object *bi_cdr(Env *env, Object *obj)
{
    NARG("cdr", obj, 1);
    EXPECT("cdr", obj, 0, O_BEXPR);
    OBJ_ENSURE(obj, obj->cell[0]->nelem, "cdr cannot operate on empty b-expression ('[]')");

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
        if (!strcmp("=", func))
            env_set(env, symbols->cell[i], list->cell[i + 1]);
        else if (!strcmp("def", func))
            env_set_global(env, symbols->cell[i], list->cell[i + 1]);
    }

    obj_free(list);

    return obj_new_sexpr();
}

Object *bi_lambda(Env *env, Object *list)
{
    NARG(">", list, 2);
    EXPECT(">", list, 0, O_BEXPR);
    EXPECT(">", list, 1, O_BEXPR);

    for (size_t i = 0; i < list->cell[0]->nelem; ++i)
        OBJ_ENSURE_F(list, list->cell[0]->cell[i]->type == O_SYMBOL,
            "can only define symbol. (got %s)", obj_type_arr[list->cell[i]->type]);

    Object *params = obj_pop(list, 0);
    Object *body = obj_pop(list, 0);

    obj_free(list);

    return obj_new_lambda(params, body);
}

// def [fn] (> [params body] [def (car params) (> (cdr params) body)])

// Object *bi_fn(Env *env, Object *list)
// {

// }

Object *obj_append(Object *obj, Object *to_add)
{
    obj->cell = s_realloc(obj->cell, sizeof(Object *) * ++obj->nelem);
    obj->cell[obj->nelem - 1] = to_add;
    return obj;
}

Object *obj_attach(Object *obj, Object *obj2)
{
    while (obj2->nelem)
        obj = obj_append(obj, obj_pop(obj2, 0));
    obj_free(obj2);
    return obj;
}

Object *obj_pop(Object *obj, size_t idx)
{
    Object *elem = obj->cell[idx];

    memmove(&obj->cell[idx], &obj->cell[idx + 1], sizeof(Object *) * (obj->nelem-- - idx - 1));
    obj->cell = s_realloc(obj->cell, sizeof(Object *) * obj->nelem);
    return elem;
}

Object *obj_take(Object *obj, size_t idx)
{
    Object *elem = obj_pop(obj, idx);
    obj_free(obj);
    return elem;
}

Object *obj_cp(Object *obj)
{
    Object *ret = s_malloc(sizeof(Object));

    ret->type = obj->type;

    switch (ret->type) {
    case O_NUMBER: ret->r.number = obj->r.number; break;
    case O_ERROR: ret->r.error = dupstr(obj->r.error); break;
    case O_SYMBOL: ret->r.symbol = dupstr(obj->r.symbol); break;
    case O_FUNC:
        if (obj->r.f.builtin == NULL) {
            ret->r.f.builtin = NULL;
            ret->r.f.env = env_cp(obj->r.f.env);
            ret->r.f.params = obj_cp(obj->r.f.params);
            ret->r.f.body = obj_cp(obj->r.f.body);
        } else {
            ret->r.f.builtin = obj->r.f.builtin;
        }
        break;
    case O_SEXPR:
    case O_BEXPR:
        ret->nelem = obj->nelem;
        ret->cell = s_malloc(ret->nelem * sizeof(Object *));
        for (size_t i = 0; i < ret->nelem; ++i)
            ret->cell[i] = obj_cp(obj->cell[i]);
        break;
    }

    return ret;
}

Object *obj_read_num(mpc_ast_T *ast)
{
	double n;
	str2dbl(&n, ast->contents);
	return errno == ERANGE
		? obj_new_err("Invalid number. Possibly out of range for double")
		: obj_new_num(n);
}

Object *obj_read(mpc_ast_T *ast)
{
	Object *v = NULL;

	if (strstr(ast->tag, "number")) return obj_read_num(ast);
	if (strstr(ast->tag, "symbol")) return obj_new_sym(ast->contents);

	if (!strcmp(ast->tag, ">") || strstr(ast->tag, "sexpression"))
		v = obj_new_sexpr();
	if (strstr(ast->tag, "bexpression"))
		v = obj_new_bexpr();

	for (int i = 0; i < ast->children_num; ++i) {
		if (!strcmp(ast->children[i]->contents, "(") ||
			!strcmp(ast->children[i]->contents, ")") ||
		    !strcmp(ast->children[i]->contents, "[") ||
			!strcmp(ast->children[i]->contents, "]") ||
			!strcmp(ast->children[i]->tag, "regex"))
			continue;
		v = obj_append(v, obj_read(ast->children[i]));
	}

	return v;
}

Object *obj_call(Env *env, Object *func, Object *list)
{
    if (func->r.f.builtin)
        return func->r.f.builtin(env, list);

    size_t
        given = list->nelem,
        total = func->r.f.params->nelem;

    while (list->nelem) {
        if (!func->r.f.params->nelem) {
            obj_free(list);
            return obj_new_err("too many arguments passed. (expected %d but %d were given)",
                total, given);
        }

        Object *symbol = obj_pop(func->r.f.params, 0);

        if (!strcmp(symbol->r.symbol, "@")) {
            if (func->r.f.params->nelem != 1) {
                obj_free(list);
                return obj_new_err("@ is missing a symbol");
            }

            Object *args = obj_pop(func->r.f.params, 0);
            env_set(func->r.f.env, args, bi_list(env, list));
            obj_free(symbol);
            obj_free(args);
            break;
        }

        Object *value = obj_pop(list, 0);

        env_set(func->r.f.env, symbol, value);

        obj_free(symbol);
        obj_free(value);
    }

    obj_free(list);

    if (func->r.f.params->nelem > 0 && *func->r.f.params->cell[0]->r.symbol == '@') {
        if (func->r.f.params->nelem != 2)
                return obj_new_err("@ is missing a symbol");
        obj_free(obj_pop(func->r.f.params, 0)); /* pop & delete '@' */

        Object *symbol = obj_pop(func->r.f.params, 0);
        Object *b = obj_new_bexpr();

        env_set(func->r.f.env, symbol, b);

        obj_free(symbol);
        obj_free(b);
    }

    if (!func->r.f.params->nelem) {
        func->r.f.env->parent = env;
        return bi_eval(func->r.f.env, obj_append(obj_new_sexpr(), obj_cp(func->r.f.body)));
    }

    return obj_cp(func);
}

Object *process_op(Env *env, Object *list, const char *op)
{
    for (size_t i = 0; i < list->nelem; ++i)
        EXPECT(op, list, i, O_NUMBER);

    Object *a = obj_pop(list, 0);

    if (*op == '-' && !list->nelem) a->r.number *= -1;
    else if (*op == '~' && !list->nelem) a->r.number = ~(int)a->r.number;

    while (list->nelem) {
        Object *b = obj_pop(list, 0);

        switch (*op) {
        case '+': a->r.number += b->r.number; break;
        case '-': a->r.number -= b->r.number; break;
        case '*': a->r.number *= b->r.number; break;
        case '/':
            if (!b->r.number) {
                obj_free(a);
                obj_free(b);
                a = obj_new_err("Division by 0 is undefined");
                goto out;
            }
            a->r.number /= b->r.number;
            break;
        case '%': a->r.number = fmod(a->r.number, b->r.number); break;
        case '^': a->r.number = (int)a->r.number ^ (int)b->r.number; break;
        case '&': a->r.number = (int)a->r.number & (int)b->r.number; break;
        case '|': a->r.number = (int)a->r.number | (int)b->r.number; break;
        }

        if (!strcmp(op, "pow")) a->r.number = pow(a->r.number, b->r.number);
        if (!strcmp(op, "min")) a->r.number = U_MIN(a->r.number, b->r.number);
        if (!strcmp(op, "max")) a->r.number = U_MAX(a->r.number, b->r.number);

        obj_free(b);
    }

out:
    obj_free(list);
    return a;
}

Object *obj_eval_sexpr(Env *env, Object *obj);

Object *obj_eval(Env *env, Object *obj)
{
    if (obj->type == O_SYMBOL) {
        Object *ret = env_get(env, obj);
        obj_free(obj);
        return ret;
    }
    return obj->type == O_SEXPR ? obj_eval_sexpr(env, obj) : obj;
}

Object *obj_eval_sexpr(Env *env, Object *obj)
{
    Object *first, *res;
    int error_id = -1;

    for (size_t i = 0; i < obj->nelem; ++i) {
        obj->cell[i] = obj_eval(env, obj->cell[i]);
        if (obj->cell[i]->type == O_ERROR)
            error_id = i;
    }

    if (error_id != -1) return obj_take(obj, error_id);

    if (!obj->nelem) return obj;
    if (obj->nelem == 1) return obj_take(obj, 0);

    if ((first = obj_pop(obj, 0))->type != O_FUNC) {
        obj_free(first);
        obj_free(obj);
        return obj_new_err("s-expression must start with a function");
    }

    res = obj_call(env, first, obj);
    obj_free(first);
    return res;
}

void obj_dump_expr(Object *obj, char open, char close)
{
    putchar(open);

    for (size_t i = 0; i < obj->nelem; ++i) {
        obj_dump(obj->cell[i]);
        if (i != obj->nelem - 1) putchar(' ');
    }

    putchar(close);
}

void obj_dump(Object *obj)
{
    switch (obj->type) {
    case O_NUMBER: printf("%g", obj->r.number); break;
    case O_ERROR: printf("Error: %s", obj->r.error); break;
    case O_SYMBOL: printf("%s", obj->r.symbol); break;
    case O_FUNC:
        if (obj->r.f.builtin) {
            printf("<built-in function>");
        } else {
            fputs("(> ", stdout);
            obj_dump(obj->r.f.params);
            putchar(' ');
            obj_dump(obj->r.f.body);
            putchar(')');
        }
        break;
    case O_SEXPR: obj_dump_expr(obj, '(', ')'); break;
    case O_BEXPR: obj_dump_expr(obj, '[', ']'); break;
    }
}

void obj_free(Object *obj)
{
    switch (obj->type) {
    case O_ERROR: free(obj->r.error); break;
    case O_SYMBOL: free(obj->r.symbol); break;
    case O_FUNC:
        if (obj->r.f.builtin == NULL) {
            env_free(obj->r.f.env);
            obj_free(obj->r.f.params);
            obj_free(obj->r.f.body);
        }
        break;
    case O_SEXPR:
    case O_BEXPR:
        for (size_t i = 0; i < obj->nelem; ++i)
            obj_free(obj->cell[i]);
        free(obj->cell);
        break;
    default: break;
    }

    free(obj);
}
