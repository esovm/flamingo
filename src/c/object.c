#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "util.h"
#include "env.h"
#include "object.h"

static const char escape_chars[] = "\a\b\t\n\v\f\r\"\'\\";
const char *const obj_type_arr[] = {
   "boolean", "number", "error", "symbol", "string", "function", "s-expression", "b-expression"
};

static char *escape(char c)
{
    switch (c) {
    case '\a': return "\\a";
    case '\b': return "\\b";
    case '\t': return "\\t";
    case '\n': return "\\n";
    case '\v': return "\\v";
    case '\f': return "\\f";
    case '\r': return "\\r";
    case '\"': return "\\\"";
    case '\'': return "\\\'";
    case '\\': return "\\\\";
    }
    return "";
}

Object *obj_new_bool(bool b)
{
    Object *ret = malloc(sizeof(Object));
    if (!ret) return NULL;
    ret->type = O_BOOLEAN;
    ret->r.boolean = b;
    return ret;
}

Object *obj_new_num(double n)
{
    Object *ret = malloc(sizeof(Object));
    if (!ret) return NULL;
    ret->type = O_NUMBER;
    ret->r.real = n;
    return ret;
}

Object *obj_new_err(const char *fmt, ...)
{
    Object *ret = malloc(sizeof(Object));
    if (!ret) return NULL;
    const int amt = 256;
    va_list ap;
    va_start(ap, fmt);

    ret->type = O_ERROR;
    if (!(ret->r.error = malloc(amt))) return NULL;

    vsnprintf(ret->r.error, amt - 1, fmt, ap);

    char *tmp = realloc(ret->r.error, strlen(ret->r.error) + 1);
    if (!tmp) return NULL;
    ret->r.error = tmp;
    va_end(ap);
    return ret;
}

Object *obj_new_raw(void *ptr)
{
    Object *ret = malloc(sizeof(Object));
    if (!ret) return NULL;
    ret->type = O_RAW;
    ret->r.rawptr = ptr;
    return ret;
}

Object *obj_new_sym(const char *s)
{
    Object *ret = malloc(sizeof(Object));
    if (!ret) return NULL;
    ret->type = O_SYMBOL;
    if (!(ret->r.symbol = dupstr(s))) return NULL;
    return ret;
}

Object *obj_new_str(const char *s)
{
    Object *ret = malloc(sizeof(Object));
    if (!ret) return NULL;
    ret->type = O_STRING;
    if (!(ret->r.string = dupstr(s))) return NULL;
    return ret;
}

Object *obj_new_func(BuiltinFn func)
{
    Object *ret = malloc(sizeof(Object));
    if (!ret) return NULL;
    ret->type = O_FUNC;
    ret->r.f.builtin = func;
    return ret;
}

Object *obj_new_lambda(Object *params, Object *body)
{
    Object *ret = malloc(sizeof(Object));
    if (!ret) return NULL;
    ret->type = O_FUNC;
    ret->r.f.builtin = NULL;
    ret->r.f.params = params;
    ret->r.f.body = body;
    ret->r.f.env = env_new();
    return ret;
}

Object *obj_new_sexpr(void)
{
    Object *ret = malloc(sizeof(Object));
    if (!ret) return NULL;
    ret->type = O_SEXPR;
    ret->nelem = 0;
    ret->cell = NULL;
    return ret;
}

Object *obj_new_bexpr(void)
{
    Object *ret = malloc(sizeof(Object));
    if (!ret) return NULL;
    ret->type = O_BEXPR;
    ret->nelem = 0;
    ret->cell = NULL;
    return ret;
}

Object *obj_append(Object *obj, Object *to_add)
{
    obj->cell = realloc(obj->cell, sizeof(Object *) * ++obj->nelem);
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

Object *obj_pop(Object *obj, int idx)
{
    Object *elem = obj->cell[idx];
    memmove(&obj->cell[idx], &obj->cell[idx + 1], sizeof(Object *) * (obj->nelem - idx - 1));
    obj->cell = realloc(obj->cell, sizeof(Object *) * --obj->nelem);
    return elem;
}

Object *obj_take(Object *obj, int idx)
{
    Object *elem = obj_pop(obj, idx);
    obj_free(obj);
    return elem;
}

Object *obj_cp(Object *obj)
{
    Object *ret = malloc(sizeof(Object));
    if (!ret) return NULL;
    ret->type = obj->type;

    switch (ret->type) {
    case O_BOOLEAN: ret->r.boolean = obj->r.boolean; break;
    case O_NUMBER: ret->r.real = obj->r.real; break;
    case O_ERROR: ret->r.error = dupstr(obj->r.error); break;
    case O_SYMBOL: ret->r.symbol = dupstr(obj->r.symbol); break;
    case O_STRING: ret->r.string = dupstr(obj->r.string); break;
    case O_FUNC:
        if (!obj->r.f.builtin) {
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
        if (!(ret->cell = malloc(ret->nelem * sizeof(Object *)))) return NULL;
        for (int i = 0; i < ret->nelem; ++i)
            ret->cell[i] = obj_cp(obj->cell[i]);
        break;
    default: break;
    }

    return ret;
}

Object *obj_call(Env *env, Object *func, Object *list)
{
    if (func->r.f.builtin) return func->r.f.builtin(env, list);

    int given = list->nelem, total = func->r.f.params->nelem;
    while (list->nelem) {
        if (!func->r.f.params->nelem) {
            obj_free(list);
            return obj_new_err("too many arguments passed. (expected %d but %d were given)",
                total, given);
        }
        Object *symbol = obj_pop(func->r.f.params, 0);
        if (strcmp(symbol->r.symbol, "@") == 0) {
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

bool obj_equal(Object *a, Object *b)
{
    if (a->type != b->type) return false;
    switch (a->type) {
    case O_BOOLEAN: return a->r.boolean == b->r.boolean;
    case O_NUMBER: return a->r.real == b->r.real;
    case O_SYMBOL: return strcmp(a->r.symbol, b->r.symbol) == 0;
    case O_STRING: return strcmp(a->r.string, b->r.string) == 0;
    case O_ERROR: return strcmp(a->r.error, b->r.error) == 0;
    case O_FUNC:
        return a->r.f.builtin || b->r.f.builtin
            ? a->r.f.builtin == b->r.f.builtin
            : obj_equal(a->r.f.params, b->r.f.params) && obj_equal(a->r.f.body, b->r.f.body);
    case O_SEXPR:
    case O_BEXPR:
        if (a->nelem != b->nelem) return false;
        for (int i = 0; i < a->nelem; ++i)
            if (!obj_equal(a->cell[i], b->cell[i])) return false;
        return true;
    default: break;
    }
    return false;
}

bool obj_is_truthy(Object *obj)
{
    switch (obj->type) {
    case O_BOOLEAN: return obj->r.boolean != false;
    case O_NUMBER: return obj->r.real != 0;
    case O_STRING: return obj->r.string[0] != '\0';
    case O_SEXPR: case O_BEXPR: return obj->nelem != 0;
    default: return true;
    }
}

Object *obj_to_bool(Object *obj)
{
    switch (obj->type) {
    case O_BOOLEAN: return obj_new_bool(obj->r.boolean);
    case O_NUMBER: return obj_new_bool(obj->r.real != 0);
    case O_STRING: return obj_new_bool(*obj->r.string != '\0');
    case O_SEXPR: case O_BEXPR: return obj_new_bool(obj->nelem != 0);
    default: return obj_new_bool(true);
    }
}

static Object *obj_eval_sexpr(Env *env, Object *obj)
{
    Object *first;
    int error_id = -1;

    for (int i = 0; i < obj->nelem; ++i) {
        obj->cell[i] = obj_eval(env, obj->cell[i]);
        if (obj->cell[i]->type == O_ERROR)
            error_id = i;
    }

    if (error_id != -1) return obj_take(obj, error_id);

    if (!obj->nelem) return obj;
    if (obj->nelem == 1) return obj_eval(env, obj_take(obj, 0));

    if ((first = obj_pop(obj, 0))->type != O_FUNC) {
        obj_free(first);
        obj_free(obj);
        return obj_new_err("s-expression must start with a function");
    }

    Object *res = obj_call(env, first, obj);
    obj_free(first);
    return res;
}

Object *obj_eval(Env *env, Object *obj)
{
    if (obj->type == O_SYMBOL) {
        Object *ret = env_get(env, obj);
        obj_free(obj);
        return ret;
    }
    return obj->type == O_SEXPR ? obj_eval_sexpr(env, obj) : obj;
}

void obj_dump_expr(Object *obj, char open, char close)
{
    putchar(open);
    for (int i = 0; i < obj->nelem; ++i) {
        obj_dump(obj->cell[i]);
        if (i != obj->nelem - 1) putchar(' ');
    }
    putchar(close);
}

void obj_dump_str(Object *obj)
{
    char c;
    for (int i = 0; (c = obj->r.string[i]); ++i)
        strchr(escape_chars, c) ? fputs(escape(c), stdout) : putchar(c);
}

void obj_dump(Object *obj)
{
    switch (obj->type) {
    case O_BOOLEAN: printf("%s", obj->r.boolean ? "true" : "false"); break;
    case O_NUMBER: printf("%g", obj->r.real); break;
    case O_ERROR: printf("[error] %s", obj->r.error); break;
    case O_SYMBOL: fputs(obj->r.symbol, stdout); break;
    case O_STRING: obj_dump_str(obj); break;
    case O_FUNC:
        if (obj->r.f.builtin) {
            printf("<built-in function>");
        } else {
            fputs("($ ", stdout);
            obj_dump(obj->r.f.params);
            putchar(' ');
            obj_dump(obj->r.f.body);
            putchar(')');
        }
        break;
    case O_SEXPR: obj_dump_expr(obj, '(', ')'); break;
    case O_BEXPR: obj_dump_expr(obj, '[', ']'); break;
    default: break;
    }
}

void obj_free(Object *obj)
{
    switch (obj->type) {
    case O_ERROR: free(obj->r.error); break;
    case O_SYMBOL: free(obj->r.symbol); break;
    case O_STRING: free(obj->r.string); break;
    case O_FUNC:
        if (!obj->r.f.builtin) {
            env_free(obj->r.f.env);
            obj_free(obj->r.f.params);
            obj_free(obj->r.f.body);
        }
        break;
    case O_SEXPR:
    case O_BEXPR:
        for (int i = 0; i < obj->nelem; ++i)
            obj_free(obj->cell[i]);
        free(obj->cell);
        break;
    default: break;
    }

    free(obj);
}
