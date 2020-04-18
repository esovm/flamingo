#ifndef FLAMINGO_OBJECT_H
#define FLAMINGO_OBJECT_H

#include <stdbool.h>

#define OBJ_ENSURE(OBJ, CONDITION, ERROR_STR)   \
    do {                                        \
        if (!(CONDITION)) {                     \
            Object *e = obj_new_err(ERROR_STR); \
            obj_free(OBJ);                      \
            return e;                           \
        }                                       \
    } while (0)

#define OBJ_ENSURE_F(OBJ, CONDITION, ERROR_STR, ...)         \
    do {                                                     \
        if (!(CONDITION)) {                                  \
            Object *e = obj_new_err(ERROR_STR, __VA_ARGS__); \
            obj_free(OBJ);                                   \
            return e;                                        \
        }                                                    \
    } while (0)

#define EXPECT(ID, OBJ, I, EXPECTED)                                               \
    OBJ_ENSURE_F(OBJ, OBJ->cell[I]->type == EXPECTED, "%s expected %s but got %s", \
                 ID, obj_type_arr[EXPECTED], obj_type_arr[OBJ->cell[I]->type]);

#define NARG(ID, OBJ, N)                                                       \
    OBJ_ENSURE_F(OBJ, OBJ->nelem == N, "%s expected %d argument%s but got %d", \
                 ID, N, N != 1 ? "s" : "", OBJ->nelem);

#define BEXPR_NOT_EMPTY(ID, OBJ, I) \
    OBJ_ENSURE_F(OBJ, OBJ->cell[I]->nelem, "%s cannot operate on empty b-expression ([]).", ID);
#define STRING_NOT_EMPTY(ID, OBJ, I) \
    OBJ_ENSURE_F(OBJ, *(OBJ->cell[I]->r.string), "%s cannot operate on empty string ('').", ID);

#define OBJ_DUMP_LN(OBJ)   \
    do {                   \
        obj_dump(OBJ);     \
        putchar('\n');     \
    } while (0)

/* these strings have to exactly match the `obj_type` enum elements */
extern const char *const obj_type_arr[];

typedef struct Object Object;
typedef struct Env Env;
typedef Object *(*BuiltinFn)(Env *, Object *);

typedef enum {
    O_BOOLEAN,
    O_NUMBER,
    O_ERROR,
    O_SYMBOL,
    O_STRING,
    O_FUNC,
    O_SEXPR,
    O_BEXPR
} obj_type;

struct Object {
    obj_type type;
    union {
        bool boolean;
        double number;
        char *error;
        char *symbol;
        char *string;
        struct {
            BuiltinFn builtin;
            Env *env;
            Object *params;
            Object *body;
        } f; /* user-defined or built-in function */
    } r; /* result */
    /* s-expression */
    int nelem;
    struct Object **cell;
};

Object *obj_new_bool(bool);
Object *obj_new_num(double);
Object *obj_new_err(const char *, ...);
Object *obj_new_sym(const char *);
Object *obj_new_str(const char *);
Object *obj_new_func(BuiltinFn);
Object *obj_new_lambda(Object *, Object *);
Object *obj_new_sexpr(void);
Object *obj_new_bexpr(void);

Object *obj_read_expr(char *, int *, char);
Object *obj_eval(Env *, Object *);
void obj_dump(Object *);
void obj_free(Object *);

Object *obj_attach(Object *, Object *);
Object *obj_append(Object *, Object *);
Object *obj_cp(Object *);
Object *obj_take(Object *, int);
Object *obj_pop(Object *, int);
bool obj_equal(Object *, Object *);
bool obj_is_truthy(Object *);
Object *obj_to_bool(Object *);

Object *read_op(Env *, Object *, const char *);
Object *read_rel(Env *, Object *, const char *);
Object *read_log(Env *, Object *, const char *);
Object *read_var(Env *, Object *, const char *);

Object *bi_exit(Env *, Object *);
Object *bi_list(Env *, Object *);
Object *bi_first(Env *, Object *);
Object *bi_last(Env *, Object *);
Object *bi_rest(Env *, Object *);
Object *bi_eval(Env *, Object *);
Object *bi_attach(Env *, Object *);
Object *bi_init(Env *, Object *);
Object *bi_lambda(Env *, Object *);
Object *bi_if(Env *, Object *);
Object *bi_use(Env *, Object *);
Object *bi_puts(Env *, Object *);
Object *bi_err(Env *, Object *);

static inline Object *bi_add(Env *env, Object *obj) {
    return read_op(env, obj, "+");
}
static inline Object *bi_sub(Env *env, Object *obj) {
    return read_op(env, obj, "-");
}
static inline Object *bi_mul(Env *env, Object *obj) {
    return read_op(env, obj, "*");
}
static inline Object *bi_div(Env *env, Object *obj) {
    return read_op(env, obj, "/");
}
static inline Object *bi_idiv(Env *env, Object *obj) { /* idiv - integer division */
    return read_op(env, obj, "//");
}
static inline Object *bi_mod(Env *env, Object *obj) {
    return read_op(env, obj, "%");
}
static inline Object *bi_bxor(Env *env, Object *obj) {
    return read_op(env, obj, "^");
}
static inline Object *bi_band(Env *env, Object *obj) {
    return read_op(env, obj, "&");
}
static inline Object *bi_bor(Env *env, Object *obj) {
    return read_op(env, obj, "|");
}
static inline Object *bi_bnot(Env *env, Object *obj) {
    return read_op(env, obj, "~");
}
static inline Object *bi_min(Env *env, Object *obj) {
    return read_op(env, obj, "min");
}
static inline Object *bi_max(Env *env, Object *obj) {
    return read_op(env, obj, "max");
}
static inline Object *bi_lt(Env *env, Object *obj) {
    return read_rel(env, obj, "<");
}
static inline Object *bi_le(Env *env, Object *obj) {
    return read_rel(env, obj, "<=");
}
static inline Object *bi_gt(Env *env, Object *obj) {
    return read_rel(env, obj, ">");
}
static inline Object *bi_ge(Env *env, Object *obj) {
    return read_rel(env, obj, ">=");
}
static inline Object *bi_eq(Env *env, Object *obj) {
    return read_rel(env, obj, "==");
}
static inline Object *bi_ne(Env *env, Object *obj) {
    return read_rel(env, obj, "!=");
}
static inline Object *bi_not(Env *env, Object *obj) {
    return read_log(env, obj, "not");
}
static inline Object *bi_or(Env *env, Object *obj) {
    return read_log(env, obj, "or");
}
static inline Object *bi_and(Env *env, Object *obj) {
    return read_log(env, obj, "and");
}
static inline Object *bi_def(Env *env, Object *list) { /* global */
    return read_var(env, list, "def");
}
static inline Object *bi_loc(Env *env, Object *list) { /* local */
    return read_var(env, list, "=");
}

#endif /* FLAMINGO_OBJECT_H */
