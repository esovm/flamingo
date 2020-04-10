#include "object.h"
#include "env.h"
#include "util.h"

/* math operators */
Object *process_op(Env *env, Object *list, const char *op)
{
    (void)env;
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

        if (EQ(op, "pow")) a->r.number = pow(a->r.number, b->r.number);
        if (EQ(op, "min")) a->r.number = U_MIN(a->r.number, b->r.number);
        if (EQ(op, "max")) a->r.number = U_MAX(a->r.number, b->r.number);

        obj_free(b);
    }

out:
    obj_free(list);
    return a;
}

/* relational operators */
Object *process_rel(Env *env, Object *list, const char *op)
{
    (void)env;
    NARG(op, list, 2);
    if (strcmp(op, "==") && strcmp(op, "!=")) {
        /* operands must be numbers only with <, <=, >, >= */
        EXPECT(op, list, 0, O_NUMBER);
        EXPECT(op, list, 1, O_NUMBER);
    }

    int r;
    switch (*op) {
    case '<':
        r = op[1] == '='
            ? list->cell[0]->r.number <= list->cell[1]->r.number
            : list->cell[0]->r.number < list->cell[1]->r.number;
        break;
    case '>':
        r = op[1] == '='
            ? list->cell[0]->r.number >= list->cell[1]->r.number
            : list->cell[0]->r.number > list->cell[1]->r.number;
        break;
    }
    if (EQ(op, "=="))
        r = obj_equal(list->cell[0], list->cell[1]);
    else if (EQ(op, "!="))
        r = !obj_equal(list->cell[0], list->cell[1]);

    obj_free(list);
    return obj_new_num(r);
}

/* logical operators */
Object *process_log(Env *env, Object *list, const char *op)
{
    (void)env;
    for (size_t i = 0; i < list->nelem; ++i)
        EXPECT(op, list, i, O_NUMBER);

    Object *a = obj_pop(list, 0);

    if (EQ(op, "not") && !list->nelem) a->r.number = !a->r.number;

    while (list->nelem) {
        Object *b = obj_pop(list, 0);

        if (EQ(op, "or")) a->r.number = a->r.number || b->r.number;
        if (EQ(op, "and")) a->r.number = a->r.number && b->r.number;

        obj_free(b);
    }

    obj_free(list);
    return a;
}

/* local and global variables */
Object *process_var(Env *env, Object *list, const char *func)
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
