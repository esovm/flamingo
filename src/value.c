#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "mpc.h"
#include "util.h"
#include "value.h"

static const char *const value_type_arr[] = { 
    "number", "error", "symbol", "s-expression", "q-expression"
};

static void *safe_malloc(size_t size)
{
    void *ret = malloc(size);
    if (!ret)
        fuk("malloc failure!");
    return ret;
}

static void *safe_realloc(void *ptr, size_t size)
{
    void *ret = realloc(ptr, size);
    if (!ret)
        fuk("realloc failure!");
    return ret;
}

Value *value_new_num(double n)
{
    Value *ret = safe_malloc(sizeof(Value));

    ret->type = VT_NUMBER;
    ret->res.number = n;

    return ret;
}

Value *value_new_err(const char *fmt, ...)
{
    Value *ret = safe_malloc(sizeof(Value));
    va_list ap;
    /* TODO: dynamically calculate buffer size rather than using an arbitrary value */
    char *buf = malloc(256);

    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);

    ret->type = VT_ERROR;
    ret->res.error = buf;

    va_end(ap);

    return ret;
}

Value *value_new_sym(const char *s)
{
    Value *ret = safe_malloc(sizeof(Value));

    ret->type = VT_SYMBOL;
    ret->res.symbol = dupstr(s);

    return ret;
}

Value *value_new_sexpr(void)
{
    Value *ret = safe_malloc(sizeof(Value));

    ret->type = VT_SEXPR;
    ret->nelem = 0;
    ret->cell = NULL;

    return ret;
}

Value *value_new_qexpr(void)
{
    Value *ret = safe_malloc(sizeof(Value));

    ret->type = VT_QEXPR;
    ret->nelem = 0;
    ret->cell = NULL;

    return ret;
}

Value *value_read_num(mpc_ast_T *ast)
{
	double n;
	str2dbl(&n, ast->contents);
	return errno == ERANGE
		? value_new_err("Invalid number. Possibly out of range for double")
		: value_new_num(n);
}

Value *value_read(mpc_ast_T *ast)
{
	Value *v = NULL;

	if (strstr(ast->tag, "number")) return value_read_num(ast);
	if (strstr(ast->tag, "symbol")) return value_new_sym(ast->contents);

	if (!strcmp(ast->tag, ">") || strstr(ast->tag, "sexpression"))
		v = value_new_sexpr();
	if (strstr(ast->tag, "qexpression"))
		v = value_new_qexpr();

	for (int i = 0; i < ast->children_num; ++i) {
		if (!strcmp(ast->children[i]->contents, "(") ||
			!strcmp(ast->children[i]->contents, ")") ||
		    !strcmp(ast->children[i]->contents, "[") ||
			!strcmp(ast->children[i]->contents, "]") ||
			!strcmp(ast->children[i]->tag, "regex"))
			continue;
		v = value_append(v, value_read(ast->children[i]));
	}

	return v;
}

Value *value_append(Value *val, Value *to_add)
{
    val->cell = safe_realloc(val->cell, sizeof(Value *) * ++val->nelem);
    val->cell[val->nelem - 1] = to_add;
    return val;
}

Value *value_pop(Value *val, size_t idx)
{
    Value *elem = val->cell[idx];

    memmove(&val->cell[idx], &val->cell[idx + 1], sizeof(Value *) * (val->nelem-- - idx - 1));
    val->cell = safe_realloc(val->cell, sizeof(Value *) * val->nelem);
    return elem;
}

Value *value_take(Value *val, size_t idx)
{
    Value *elem = value_pop(val, idx);
    value_free(val);
    return elem;
}

static Value *do_op(Value *list, const char *op)
{
    for (size_t i = 0; i < list->nelem; ++i)
        if (list->cell[i]->type != VT_NUMBER) {
            value_free(list);
            return value_new_err("Evaluation on non-number is not allowed");
        }

    Value *a = value_pop(list, 0);

    if (*op == '-' && !list->nelem) a->res.number *= -1;
    else if (*op == '~' && !list->nelem) a->res.number = ~(int)a->res.number;

    while (list->nelem) {
        Value *b = value_pop(list, 0);

        switch (*op) {
        case '+': a->res.number += b->res.number; break;
        case '-': a->res.number -= b->res.number; break;
        case '*': a->res.number *= b->res.number; break;
        case '/':
            if (!b->res.number) {
                value_free(a);
                value_free(b);
                a = value_new_err("Division by 0 is undefined");
                goto out;
            }
            a->res.number /= b->res.number;
            break;
        case '%': a->res.number = fmod(a->res.number, b->res.number); break;
        case '^': a->res.number = pow(a->res.number, b->res.number); break;
        case '&': a->res.number = (int)a->res.number & (int)b->res.number; break;
        case '|': a->res.number = (int)a->res.number | (int)b->res.number; break;
        }

        if (!strcmp(op, "min")) a->res.number = U_MIN(a->res.number, b->res.number);
        if (!strcmp(op, "max")) a->res.number = U_MAX(a->res.number, b->res.number);

        value_free(b);
    }

out:
    value_free(list);
    return a;
}

static Value *do_car(Value *val)
{
    const char *got = value_type_arr[val->cell[0]->type];

    VALUE_ENSURE(val->nelem == 1, "car expected 1 argument but got %d", val->nelem);
    VALUE_ENSURE(val->cell[0]->type == VT_QEXPR, "car expected q-expression but got %s", got);
    VALUE_ENSURE(val->cell[0]->nelem, "car cannot operate on empty q-expression ('[]')", "");

    Value *first = value_take(val, 0);
    while (first->nelem > 1)
        value_free(value_pop(first, 1));
    return first;
}

static Value *do_cdr(Value *val)
{
    const char *got = value_type_arr[val->cell[0]->type];
    
    VALUE_ENSURE(val->nelem == 1, "cdr expected 1 argument but got %d", val->nelem);
    VALUE_ENSURE(val->cell[0]->type == VT_QEXPR, "cdr expected q-expression but got %s", got);
    VALUE_ENSURE(val->cell[0]->nelem, "cdr cannot operate on empty q-expression ('[]')", "");

    Value *first = value_take(val, 0);
    value_free(value_pop(first, 0));
    return first;
}

static Value *do_list(Value *val)
{
    val->type = VT_QEXPR;
    return val;
}

static Value *do_eval(Value *val)
{
    const char *got = value_type_arr[val->cell[0]->type];

    VALUE_ENSURE(val->nelem == 1, "eval expected 1 argument but got %d", val->nelem);
    VALUE_ENSURE(val->cell[0]->type == VT_QEXPR, "eval expected q-expression but got %s", got);

    Value *ret = value_take(val, 0);
    ret->type = VT_SEXPR;
    return value_eval(ret);
}

static Value *value_attach(Value *val, Value *val2)
{
    while (val2->nelem)
        val = value_append(val, value_pop(val2, 0));
    value_free(val2);
    return val;
}

static Value *do_attach(Value *val)
{
    for (size_t i = 0; i < val->nelem; ++i) {
        const char *got = value_type_arr[val->cell[i]->type];
        VALUE_ENSURE(val->cell[i]->type == VT_QEXPR, "attach expected q-expression but got %s", got);
    }

    Value *ret = value_pop(val, 0);
    while (val->nelem)
        ret = value_attach(ret, value_pop(val, 0));

    value_free(val);
    return ret;
}

/* return q-expression without the last element */
static Value *do_init(Value *val)
{
    const char *got = value_type_arr[val->cell[0]->type];

    VALUE_ENSURE(val->nelem == 1, "init expected 1 argument but got %d", val->nelem);
    VALUE_ENSURE(val->cell[0]->type == VT_QEXPR, "init expected q-expression but got %s", got);
    VALUE_ENSURE(val->cell[0]->nelem, "init cannot operate on empty q-expression ('[]')", "");

    Value *first = value_take(val, 0);
    value_free(value_pop(first, first->nelem - 1));
    return first;
}

static Value *do_len(Value *val)
{
    size_t len = 0;

    for (size_t i = 0; i < val->nelem; ++i) {
        len += i;
    }

    return val;
}

Value *value_eval(Value *val)
{
    return val->type == VT_SEXPR ? value_eval_sexpr(val) : val;
}

Value *value_eval_sexpr(Value *val)
{
    Value *first, *res;
    int error_id = -1;

    for (size_t i = 0; i < val->nelem; ++i) {
        val->cell[i] = value_eval(val->cell[i]);
        if (val->cell[i]->type == VT_ERROR)
            error_id = i;
    }

    if (error_id != -1) return value_take(val, error_id);

    if (!val->nelem) return val;
    if (val->nelem == 1) return value_take(val, 0);

    if ((first = value_pop(val, 0))->type != VT_SYMBOL) {
        value_free(first);
        value_free(val);
        return value_new_err("s-expression must start with a symbol");
    }

    res = dof(val, first->res.symbol);
    value_free(first);
    return res;
}

/* dof - process functions */
Value *dof(Value *val, const char *f)
{
    if (!strcmp(f, "list")) return do_list(val);
    if (!strcmp(f, "car")) return do_car(val);
    if (!strcmp(f, "cdr")) return do_cdr(val);
    if (!strcmp(f, "init")) return do_init(val);
    if (!strcmp(f, "attach")) return do_attach(val);
    if (!strcmp(f, "eval")) return do_eval(val);
    if (!strcmp(f, "len")) return do_len(val);
    if (strstr("+-*/%^&|~", f) || !strcmp(f, "min") || !strcmp(f, "max")) return do_op(val, f);

    value_free(val);
    return value_new_err("Unrecognized function '%s'", f);
}

void value_dump_expr(Value *val, char open, char close)
{
    putchar(open);

    for (size_t i = 0; i < val->nelem; ++i) {
        value_dump(val->cell[i]);
        if (i != val->nelem - 1) putchar(' ');
    }

    putchar(close);
}

void value_dump(Value *val)
{
    switch (val->type) {
    case VT_NUMBER: printf("%g", val->res.number); break;
    case VT_ERROR: printf("Error: %s", val->res.error); break;
    case VT_SYMBOL: printf("%s", val->res.symbol); break;
    case VT_SEXPR: value_dump_expr(val, '(', ')'); break;
    case VT_QEXPR: value_dump_expr(val, '[', ']'); break;
    }
}

void value_free(Value *val)
{
    switch (val->type) {
    case VT_ERROR: free(val->res.error); break;
    case VT_SYMBOL: free(val->res.symbol); break;
    case VT_SEXPR:
    case VT_QEXPR:
        for (size_t i = 0; i < val->nelem; ++i)
            value_free(val->cell[i]);
        free(val->cell);
        break;
    default: break;
    }

    free(val);
}
