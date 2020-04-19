#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <errno.h>

#include "env.h"
#include "util.h"
#include "object.h"

#define SYM_OR_NUM(s, i) (isalnum(s[i]) || strchr("_+-*/=<>$#%^&@~|!.\\", s[i]))

static const char unescape_chars[] = "abtnvfr\"\'\\";

/* skip whitespace and comments */
static void skip_whitespace(const char *str, int *pos)
{
    while ((isspace(str[*pos]) || str[*pos] == '#') && str[*pos]) {
        if (str[*pos] == '#')
            while (str[*pos] != '\n' && str[*pos])
                ++*pos;
        ++*pos;
    }
}

static char unescape(char c)
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

/* math operators */
Object *read_op(Env *env, Object *list, const char *op)
{
    UNUSED(env);
    for (int i = 0; i < list->nelem; ++i)
        EXPECT(op, list, i, O_NUMBER);

    Object *a = obj_pop(list, 0);

    if (*op == '-' && !list->nelem) a->r.real *= -1;
    else if (*op == '~' && !list->nelem) a->r.real = ~(int)a->r.real;

    while (list->nelem) {
        Object *b = obj_pop(list, 0);

        switch (*op) {
        case '+': a->r.real += b->r.real; break;
        case '-': a->r.real -= b->r.real; break;
        case '*': a->r.real *= b->r.real; break;
        case '/':
            if (b->r.real == 0) {
                obj_free(a);
                obj_free(b);
                a = obj_new_err("Division by 0 is undefined");
                goto out;
            }
            a->r.real = op[1] == '/' ? (int)a->r.real / (int)b->r.real : a->r.real / b->r.real;
            break;
        case '%': a->r.real = fmod(a->r.real, b->r.real); break;
        case '^': a->r.real = (int)a->r.real ^ (int)b->r.real; break;
        case '&': a->r.real = (int)a->r.real & (int)b->r.real; break;
        case '|': a->r.real = (int)a->r.real | (int)b->r.real; break;
        }
        if (strcmp(op, "min") == 0) a->r.real = a->r.real < b->r.real ? a->r.real : b->r.real;
        if (strcmp(op, "max") == 0) a->r.real = a->r.real > b->r.real ? a->r.real : b->r.real;
        obj_free(b);
    }

out:
    obj_free(list);
    return a;
}

/* relational operators */
Object *read_rel(Env *env, Object *list, const char *op)
{
    UNUSED(env);
    NARG(op, list, 2);
    if (strcmp(op, "==") && strcmp(op, "!=")) {
        /* operands must be numbers only with <, <=, >, >= */
        EXPECT(op, list, 0, O_NUMBER);
        EXPECT(op, list, 1, O_NUMBER);
    }

    bool r = false;
    switch (*op) {
    case '<':
        r = op[1] == '='
            ? list->cell[0]->r.real <= list->cell[1]->r.real
            : list->cell[0]->r.real < list->cell[1]->r.real;
        break;
    case '>':
        r = op[1] == '='
            ? list->cell[0]->r.real >= list->cell[1]->r.real
            : list->cell[0]->r.real > list->cell[1]->r.real;
        break;
    }
    if (strcmp(op, "==") == 0)
        r = obj_equal(list->cell[0], list->cell[1]);
    else if (strcmp(op, "!=") == 0)
        r = !obj_equal(list->cell[0], list->cell[1]);

    obj_free(list);
    return obj_new_bool(r);
}

/* logical operators */
Object *read_log(Env *env, Object *list, const char *op)
{
    UNUSED(env);
    Object *a = obj_to_bool(obj_pop(list, 0));

    if (strcmp(op, "not") == 0 && !list->nelem) a->r.boolean = !a->r.boolean;

    while (list->nelem) {
        Object *b = obj_to_bool(obj_pop(list, 0));

        if (strcmp(op, "or") == 0) a->r.boolean = a->r.boolean || b->r.boolean;
        else if (strcmp(op, "and") == 0) a->r.boolean = a->r.boolean && b->r.boolean;

        obj_free(b);
    }

    obj_free(list);
    return a;
}

/* local and global variables */
Object *read_var(Env *env, Object *list, const char *func)
{
    EXPECT(func, list, 0, O_BEXPR);

    Object *symbols = list->cell[0];
    for (int i = 0; i < symbols->nelem; ++i)
        OBJ_ENSURE_F(list, symbols->cell[i]->type == O_SYMBOL, "can only define symbol (got %s)",
            obj_type_arr[symbols->cell[i]->type]);

    OBJ_ENSURE_F(list, symbols->nelem == list->nelem - 1,
        "incorrect real of arguments (%d) for %s", list->nelem - 1, func);

    for (int i = 0; i < symbols->nelem; ++i) {
        if (*func == '=')
            env_set(env, symbols->cell[i], list->cell[i + 1]);
        else if (strcmp("def", func) == 0)
            env_set_global(env, symbols->cell[i], list->cell[i + 1]);
    }

    obj_free(list);

    return obj_new_sexpr();
}

static Object *obj_read_num(const char *str)
{
    errno = 0;
    char *end;
    if (*str == '0' && (str[1] == 'b' || str[1] == 'x')) {
        long n = strtol(str+2, &end, str[1] == 'b' ? 2 : 16);
        if (errno == ERANGE || *end != '\0')
            return obj_new_err("malformed %s number", str[1] == 'b' ? "binary" : "hexadecimal");
        return obj_new_num(n);
    }
    /* Decimal otherwise */
    double n = strtod(str, &end);
    if (errno == ERANGE || *end != '\0')
        return obj_new_err("malformed number");
    return obj_new_num(n);
}

static Object *obj_read_sym(const char *str, int *pos)
{
    int tmp_size = 10, len = 0;
    char *tmp = calloc(tmp_size, 1);
    if (!tmp) return NULL;
    while (SYM_OR_NUM(str, *pos) && str[*pos]) {
        tmp[len++] = str[(*pos)++];
        if ((len + 2) >= tmp_size) {
            tmp_size *= 2;
            tmp = realloc(tmp, tmp_size);
        }
    }
    tmp[len] = '\0';
    bool is_num = strchr("-.", *tmp) || (*tmp == '0' && (tmp[1] == 'b' || tmp[1] == 'x')) || isdigit(*tmp);
    int base = 10;
    if (*tmp == '0') {
        if (tmp[1] == 'b') base = 2;
        else if (tmp[1] == 'x') base = 16;
        if (tmp[2] == '\0') is_num = false;
    }

    for (int i = (base != 10 ? 2 : 1); tmp[i]; ++i) {
        if (tmp[i] == '.') break;
        if (!is_valid_number_part(tmp[i], base)) {
            is_num = false;
            break;
        }
    }
    if (!tmp[1] && (strchr("-.", *tmp))) is_num = false; /* it's just a minus or dot */

    Object *ret;
    if (is_num) {
        ret = obj_read_num(tmp);
    } else if (strcmp(tmp, "true") == 0) {
        ret = obj_new_bool(true);
    } else if (strcmp(tmp, "false") == 0) {
        ret = obj_new_bool(false);
    } else {
        ret = obj_new_sym(tmp);
    }
    free(tmp);
    return ret;
}

static Object *obj_read_str(char *str, int *pos)
{
    int tmp_size = 10, len = 0;
    char *tmp = calloc(tmp_size, 1);
    if (!tmp) return NULL;

    ++*pos; /* skip opening quote */

    while (str[*pos] != '\'') {
        char c = str[*pos];
        if (!c) {
            free(tmp);
            return obj_new_err("string literal is missing an opening/closing quote");
        }
        if (c == '\\') {
            ++*pos; /* advance to character after backslash */
            if (strchr(unescape_chars, str[*pos])) {
                c = unescape(str[*pos]);
            } else {
                free(tmp);
                return obj_new_err("unknown escape sequence '\\%c'", str[*pos]);
            }
        }
        tmp[len++] = c;
        if ((len + 2) >= tmp_size) {
            tmp_size *= 2;
            tmp = realloc(tmp, tmp_size);
        }
        tmp[len] = '\0';
        ++*pos;
    }

    ++*pos; /* skip closing quote */
    Object *ret = obj_new_str(tmp);
    free(tmp);
    return ret;
}

static Object *obj_read(char *, int *);

Object *obj_read_expr(char *str, int *pos, char end)
{
    Object *a = end == ']' ? obj_new_bexpr() : obj_new_sexpr(), *b;

    while (str[*pos] != end) {
        if ((b = obj_read(str, pos))->type == O_ERROR) {
            obj_free(a);
            return b;
        } else {
            obj_append(a, b);
        }
    }
    ++*pos; /* skip 'end' char */
    return a;
}

Object *obj_read(char *str, int *pos)
{
    Object *ret;

    skip_whitespace(str, pos);

    if (!str[*pos]) return obj_new_err("unexpected EOF or end of input");

    if (str[*pos] == '(') {
        ++*pos;
        ret = obj_read_expr(str, pos, ')');
    } else if (str[*pos] == '[') {
        ++*pos;
        ret = obj_read_expr(str, pos, ']');
    } else if (SYM_OR_NUM(str, *pos)) {
        ret = obj_read_sym(str, pos);
    } else if (str[*pos] == '\'') {
        ret = obj_read_str(str, pos);
    } else {
        ret = obj_new_err("i don't know how to handle '%c'", str[*pos]);
    }

    skip_whitespace(str, pos);

    return ret;
}
