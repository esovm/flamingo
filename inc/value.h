#ifndef VALUE_H
#define VALUE_H

#define VALUE_ENSURE(condition, error_string, ...)           \
    do {                                                     \
        if (!(condition)) {                                  \
            value_free(val);                                 \
            return value_new_err(error_string, __VA_ARGS__); \
        }                                                    \
    } while (0)

typedef enum {
    VT_NUMBER,
    VT_ERROR,
    VT_SYMBOL,
    VT_SEXPR,
    VT_QEXPR
} value_type;

typedef struct Value {
    value_type type;
    union {
        double number;
        char *error;
        char *symbol;
    } res;
    size_t nelem;
    struct Value **cell;
} Value;

Value *value_new_num(double n);
Value *value_new_err(const char *fmt, ...);
Value *value_new_sym(const char *s);
Value *value_new_sexpr(void);
Value *value_new_qexpr(void);
Value *value_read_num(mpc_ast_T *ast);
Value *value_read(mpc_ast_T *ast);
Value *value_append(Value *val, Value *to_add);
Value *value_eval_sexpr(Value *val);
Value *value_eval(Value *val);
Value *dof(Value *val, const char *f);
void value_dump_expr(Value *val, char open, char close);
void value_dump(Value *val);
void value_free(Value *val);

#endif
