#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>

#include "util.h"
#include "object.h"
#include "env.h"

#define VALID_CHARS "_+-*/=<>%^\\!&@|~$"
#define SYM_NUM(s, i) (isalnum(s[i]) || strchr(VALID_CHARS, s[i]))

static const char escape_chars[] = "\a\b\t\n\v\f\r\"\'\\";
static const char unescape_chars[] = "abtnvfr\"\'\\";

static char *obj_escape(char c)
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

static char obj_unescape(char c)
{
    switch (c) {
    case 'a': return '\a';
    case 'b': return '\b';
    case 't': return '\t';
    case 'n': return '\n';
    case 'v': return '\v';
    case 'f': return '\f';
    case 'r': return '\r';
    case '\"': return '\"';
    case '\'': return '\'';
    case '\\': return '\\';
    }
    return '\0';
}

/* these strings have to exactly match the `obj_type` enum elements */
const char *const obj_type_arr[] = {
   "boolean", "number", "error", "symbol", "string", "function", "s-expression", "b-expression"
};

Object *obj_new_bool(bool b)
{
    Object *ret = s_malloc(sizeof(Object));

    ret->type = O_BOOLEAN;
    ret->r.boolean = b;

    return ret;
}

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

Object *obj_new_str(const char *s)
{
    Object *ret = s_malloc(sizeof(Object));

    ret->type = O_STRING;
    ret->r.string = dupstr(s);

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
    memmove(&obj->cell[idx], &obj->cell[idx + 1], sizeof(Object *) * (obj->nelem - idx - 1));
    obj->cell = s_realloc(obj->cell, sizeof(Object *) * --obj->nelem);
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
    case O_BOOLEAN: ret->r.boolean = obj->r.boolean; break;
    case O_NUMBER: ret->r.number = obj->r.number; break;
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
        ret->cell = s_malloc(ret->nelem * sizeof(Object *));
        for (size_t i = 0; i < ret->nelem; ++i)
            ret->cell[i] = obj_cp(obj->cell[i]);
        break;
    }

    return ret;
}

static Object *obj_read_bool(char *s)
{
	return obj_new_bool(EQ(s, "true") ? true : false);
}

static Object *obj_read_num(char *s)
{
    double n;
    str2dbl(&n, s);
    return errno == ERANGE
        ? obj_new_err("Invalid number. Possibly out of range for a C double")
        : obj_new_num(n);
}

static Object *obj_read_sym(char *string, size_t *pos)
{
    char *tmp = calloc(1, 1), c;

    while (SYM_NUM(string, *pos) && string[*pos]) {
        tmp = s_realloc(tmp, strlen(tmp) + 2); /* + 2, 1 for character, 1 for null term */
        tmp[strlen(tmp)] = string[*pos++];
        tmp[strlen(tmp) + 1] = '\0';
        // ++pos;
    }

    bool number = *tmp == '-' || isdigit(*tmp);
    while ((c = *tmp++)) {
        if (!isdigit(c)) {
            number = false;
            break;
        }
    }

    Object *ret;
    obj_append(ret, number ? obj_read_num(tmp) : obj_new_sym(tmp));
    free(tmp);
    return ret;
}

static Object *obj_read_str(char *string, size_t *pos)
{
    char *tmp = calloc(1, 1), c;

    (*pos)++;

    while (string[*pos] != '\'') {
        c = string[*pos];
        if (!c) {
            free(tmp);
            return obj_new_err("string literal is missing a closing quote");
        }
        if (c == '\\') {
            if (strchr(unescape_chars, string[++pos])) {
                c = obj_unescape(string[pos]);
            } else {
                obj_append(obj, obj_new_err("unknown escape sequence '\\%c'", c));
                free(tmp);
                return strlen(string);
            }
        }

        tmp = s_realloc(tmp, strlen(tmp) + 2);
        tmp[strlen(tmp)] = c;
        tmp[strlen(tmp) + 1] = '\0';
        ++pos;
    }

    obj_append(obj, obj_new_str(tmp));
    free(tmp);
    return pos + 1;
}

Object *obj_read_expr(char *string, size_t pos, char end)
{
    while (string[pos] != end) {
        if (!string[pos]) {
            obj_append(out, obj_new_err("missing %c at end of input", end));
            return strlen(string) + 1;
        }
        if (isspace((unsigned char)string[pos])) { /* whitespace */
            /* skip it and continue */
            ++pos;
            continue;
        }
        if (string[pos] == '#') { /* single line comment */
            while (string[pos] != '\n' && string[pos]) ++pos;
            ++pos;
            continue;
        }
        if (string[pos] == '(' || string[pos] == '[') { /* s-expression or b-expression */
            Object *e = string[pos] == '(' ? obj_new_sexpr() : obj_new_bexpr();
            obj_append(out, e);
            pos = obj_read_expr(e, string, pos + 1, string[pos] == '(' ? ')' : ']');
            continue;
        }
        if (SYM_NUM(string, pos)) {
            /* number or symbol */
            pos = obj_read_sym(out, string, pos);
            continue;
        }
        if (string[pos] == '\'') { /* string */
            pos = obj_read_str(out, string, pos + 1);
            continue;
        }

        obj_append(out, obj_new_err("I don't know how to handle '%c'", string[pos]));
        return strlen(string) + 1;
    }
    return pos + 1;
}

Object *obj_call(Env *env, Object *func, Object *list)
{
    if (func->r.f.builtin)
        return func->r.f.builtin(env, list);

    size_t given = list->nelem, total = func->r.f.params->nelem;

    while (list->nelem) {
        if (!func->r.f.params->nelem) {
            obj_free(list);
            return obj_new_err("too many arguments passed. (expected %d but %d were given)",
                total, given);
        }

        Object *symbol = obj_pop(func->r.f.params, 0);

        if (EQ(symbol->r.symbol, "@")) {
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
    case O_NUMBER: return a->r.number == b->r.number;
    case O_SYMBOL: return EQ(a->r.symbol, b->r.symbol);
    case O_STRING: return EQ(a->r.string, b->r.string);
    case O_ERROR: return EQ(a->r.error, b->r.error);
    case O_FUNC:
        return a->r.f.builtin || b->r.f.builtin
            ? a->r.f.builtin == b->r.f.builtin
            : obj_equal(a->r.f.params, b->r.f.params) && obj_equal(a->r.f.body, b->r.f.body);
    case O_SEXPR:
    case O_BEXPR:
        if (a->nelem != b->nelem) return false;
        for (size_t i = 0; i < a->nelem; ++i)
            if (!obj_equal(a->cell[i], b->cell[i])) return false;
        return true;
    }
}

bool obj_is_truthy(Object *obj)
{
    switch (obj->type) {
    case O_BOOLEAN: return obj->r.boolean != false;
    case O_NUMBER: return obj->r.number != 0;
    case O_STRING: return obj->r.string[0] != '\0';
    case O_SEXPR: case O_BEXPR: return obj->nelem != 0;
    default: return true;
    }
}

Object *obj_to_bool(Object *obj)
{
    switch (obj->type) {
    case O_BOOLEAN: return obj_new_bool(obj->r.boolean);
    case O_NUMBER: return obj_new_bool(obj->r.number != 0);
    case O_STRING: return obj_new_bool(*obj->r.string != '\0');
    case O_SEXPR: case O_BEXPR: return obj_new_bool(obj->nelem != 0);
    default: return obj_new_bool(true);
    }
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

void obj_dump_str(Object *obj)
{
    char c;
    putchar('\'');
    while ((c = *obj->r.string++))
        printf(strchr(escape_chars, c) ? "%s" : "%c", strchr(escape_chars, c) ? obj_escape(c) : c);
    putchar('\'');
}

void obj_dump(Object *obj)
{
    switch (obj->type) {
    case O_BOOLEAN: printf("%s", obj->r.boolean ? "true" : "false"); break;
    case O_NUMBER: printf("%g", obj->r.number); break;
    case O_ERROR: printf("Error: %s", obj->r.error); break;
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
        for (size_t i = 0; i < obj->nelem; ++i)
            obj_free(obj->cell[i]);
        free(obj->cell);
        break;
    default: break;
    }

    free(obj);
}
