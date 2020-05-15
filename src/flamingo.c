#include <string.h>
#include <dlfcn.h>

#include "gc.h"
#include "lib.h"
#include "type.h"
#include "flamingo.h"

/* IMPORTANT: this enum and `builtins` array element order must match */
enum {
    BI_LET, BI_SET, BI_IF, BI_FN, BI_MACRO, BI_USE, BI_WHILE, BI_QUOTE, BI_EVAL, BI_TYPE, BI_AND,
    BI_OR, BI_DO, BI_CONS, BI_FIRST, BI_REST, BI_SETF, BI_SETR, BI_LIST, BI_NOT, BI_ATOM, BI_PRINT,
    BI_EQ, BI_LT, BI_LE, BI_GT, BI_GE, BI_ADD, BI_SUB, BI_MUL, BI_DIV, BI_LEN
};

static const char *const builtins[BI_LEN] = {
    "let", "set", "if", "fn", "macro", "use", "while", "quote", "eval", "type", "and",
    "or", "do", "cons", "first", "rest", "setf", "setr", "list", "not", "atom", "print",
    "=", "<", "<=", ">", ">=", "+", "-", "*", "/"
};

static const char *const types[] = {
    "pair", "free", "nil", "number",
    "symbol", "string", "function", "macro",
    "built-in", "c-function", "pointer"
};

Fl_Object nil = { { (void *)(T_NIL << 2 | 1) }, { NULL } };

Fl_Handlers *Fl_handlers(Fl_Context *ctx) {
    return &ctx->handlers;
}

void Fl_error(Fl_Context *ctx, const char *message) {
    Fl_Object *cl = ctx->call_list;
    /* reset context state */
    ctx->call_list = &nil;
    /* call custom error handler if there is one */
    if (ctx->handlers.error)
        ctx->handlers.error(ctx, message, cl);
    /* error handler returned - print error, traceback and exit unsuccessfully */
    fprintf(stderr, "[error] %s\n", message);
    while (!M_isnil(cl)) {
        char buf[MAX_BUF_LEN];
        Fl_to_string(ctx, M_first(cl), buf, sizeof(buf));
        fprintf(stderr, "-> %s\n", buf);
        cl = M_rest(cl);
    }
    exit(EXIT_FAILURE);
}

Fl_Object *Fl_next_arg(Fl_Context *ctx, Fl_Object **arg) {
    Fl_Object *a = *arg;
    if (M_type(a) != T_PAIR) {
        Fl_error(ctx, M_isnil(a) ? "not enough arguments" :
            "are you nuts? there's dotted pair in your argument list!");
    }
    *arg = M_rest(a);
    return M_first(a);
}

static Fl_Object *p_check_type(Fl_Context *ctx, Fl_Object *obj, Fl_Type type) {
    if ((Fl_Type)M_type(obj) != type) {
        char buf[MAX_BUF_LEN];
        snprintf(buf, sizeof(buf), "expected %s but got %s", types[type], types[M_type(obj)]);
        Fl_error(ctx, buf);
    }
    return obj;
}

Fl_Type Fl_type(Fl_Context *ctx, Fl_Object *obj) {
    M_unused(ctx);
    return M_type(obj);
}

bool Fl_isnil(Fl_Context *ctx, Fl_Object *obj) {
    M_unused(ctx);
    return M_isnil(obj);
}

static bool p_equal(Fl_Object *x, Fl_Object *y) {
    if (x == y)
        return true;
    if (M_type(x) != M_type(y))
        return false;
    if (M_type(x) == T_NUMBER)
        return M_number(x) == M_number(y);
    if (M_type(x) == T_STRING) {
        for (; !M_isnil(x); x = M_rest(x), y = M_rest(y))
            if (M_first(x) != M_first(y))
                return false;
        return x == y;
    }
    return false;
}

bool Fl_str_equal(Fl_Object *obj, const char *str) {
    while (!M_isnil(obj)) {
        for (int i = 0; i < STR_BUF_SIZE; ++i) {
            if (M_strbuf(obj)[i] != *str)
                return false;
            if (*str)
                ++str;
        }
        obj = M_rest(obj);
    }
    return !*str;
}

Fl_Object *Fl_object(Fl_Context *ctx) {
    /* collect garbage if free list doesn't have any more objects */
    if (M_isnil(ctx->free_list)) {
        Fl_Gc_collect(ctx);
        if (M_isnil(ctx->free_list))
            Fl_error(ctx, "I'm out of memory :(");
    }
    /* get object from free list and push it to the stack */
    Fl_Object *obj = ctx->free_list;
    ctx->free_list = M_rest(obj);
    Fl_Gc_push(ctx, obj);
    return obj;
}

Fl_Object *Fl_first(Fl_Context *ctx, Fl_Object *obj) {
    return M_isnil(obj) ? obj : M_first(p_check_type(ctx, obj, T_PAIR));
}

Fl_Object *Fl_rest(Fl_Context *ctx, Fl_Object *obj) {
    return M_isnil(obj) ? obj : M_rest(p_check_type(ctx, obj, T_PAIR));
}

Fl_Object *Fl_list(Fl_Context *ctx, Fl_Object **objects, int n) {
    Fl_Object *res = &nil;
    while (n--)
        res = Fl_T_cons(ctx, objects[n], res);
    return res;
}

static void p_str_write(Fl_Context *ctx, Fl_Write_fn wfn, void *data, const char *s) {
    while (*s)
        wfn(ctx, data, *s++);
}

void Fl_write(Fl_Context *ctx, Fl_Object *obj, Fl_Write_fn wfn, void *data, bool with_quotes) {
    char buf[MAX_BUF_LEN / 2];
    switch (M_type(obj)) {
    case T_NIL:
        p_str_write(ctx, wfn, data, "nil");
        break;
    case T_NUMBER:
        snprintf(buf, sizeof(buf), "%.14g", M_number(obj));
        p_str_write(ctx, wfn, data, buf);
        break;
    case T_PAIR:
        wfn(ctx, data, '(');
        while (true) {
            Fl_write(ctx, M_first(obj), wfn, data, true);
            obj = M_rest(obj);
            if (M_type(obj) != T_PAIR)
                break;
            wfn(ctx, data, ' ');
        }
        if (!M_isnil(obj)) {
            p_str_write(ctx, wfn, data, " . ");
            Fl_write(ctx, obj, wfn, data, true);
        }
        wfn(ctx, data, ')');
        break;
    case T_SYMBOL:
        Fl_write(ctx, M_first(M_rest(obj)), wfn, data, false);
        break;
    case T_STRING:
        if (with_quotes)
            wfn(ctx, data, '"');
        while (!M_isnil(obj)) {
            for (int i = 0; i < STR_BUF_SIZE && M_strbuf(obj)[i]; ++i) {
                if (with_quotes && M_strbuf(obj)[i] == '"')
                    wfn(ctx, data, '\\');
                wfn(ctx, data, M_strbuf(obj)[i]);
            }
            obj = M_rest(obj);
        }
        if (with_quotes)
            wfn(ctx, data, '"');
        break;
    default:
        snprintf(buf, sizeof(buf), "[%s at %p]", types[M_type(obj)], (void *)obj);
        p_str_write(ctx, wfn, data, buf);
        break;
    }
}

static void p_writefp(Fl_Context *ctx, void *data, char c) {
    M_unused(ctx);
    fputc(c, data);
}

void Fl_writefp(Fl_Context *ctx, Fl_Object *obj, FILE *fp) {
    Fl_write(ctx, obj, p_writefp, fp, false);
}

typedef struct {
    char *s;
    int n;
} p_StrInt;

static void p_writebuf(Fl_Context *ctx, void *data, char c) {
    M_unused(ctx);
    p_StrInt *si = data;
    if (si->n) {
        *si->s++ = c;
        --si->n;
    }
}

int Fl_to_string(Fl_Context *ctx, Fl_Object *obj, char *buf, int size) {
    p_StrInt si = { buf, size - 1 };
    Fl_write(ctx, obj, p_writebuf, &si, false);
    *si.s = '\0';
    return size - si.n - 1;
}

Fl_Number Fl_to_number(Fl_Context *ctx, Fl_Object *obj) {
    return M_number(p_check_type(ctx, obj, T_NUMBER));
}

void *Fl_to_ptr(Fl_Context *ctx, Fl_Object *obj) {
    return M_rest(p_check_type(ctx, obj, T_PTR));
}

static Fl_Object *p_get_bound(Fl_Object *sym, Fl_Object *env) {
    /* try to find the symbol in the "environment" first */
    for (; !M_isnil(env); env = M_rest(env)) {
        Fl_Object *o = M_first(env);
        if (M_first(o) == sym)
            return o;
    }
    /* symbol couldn't be found, return global */
    return M_rest(sym);
}

void Fl_set(Fl_Context *ctx, Fl_Object *sym, Fl_Object *value) {
    M_unused(ctx);
    M_rest(p_get_bound(sym, &nil)) = value;
}

static Fl_Object rpr; /* ")" */

static Fl_Object *p_read(Fl_Context *ctx, Fl_Read_fn rfn, void *data) {
    Fl_Object *value, *res;

    /* get next char */
    int c = ctx->next_char ? ctx->next_char : rfn(ctx, data);
    ctx->next_char = '\0';

    /* skip whitespace, who needs it? */
    while (c && strchr(" \n\t\r\v\f", c))
        c = rfn(ctx, data);

    switch (c) {
    case '\0':
        return NULL;
    case '#': /* single-line comment */
        while (c && c != '\n')
            c = rfn(ctx, data);
        return p_read(ctx, rfn, data); /* you gotta love seeing recursion */
    case ')':
        return &rpr;
    case '(':
        res = &nil;
        Fl_Object **tail = &res;
        int gc = Fl_Gc_save(ctx);
        Fl_Gc_push(ctx, res); /* error on too-deep nesting */
        while ((value = p_read(ctx, rfn, data)) != &rpr) {
            if (value == NULL)
                Fl_error(ctx, "list is missing a closing parenthesis ')', remember, you need to close it!");
            if (M_type(value) == T_SYMBOL && Fl_str_equal(M_first(M_rest(value)), ".")) {
                /* dotted pair */
                *tail = Fl_read(ctx, rfn, data);
            } else {
                /* proper pair */
                *tail = Fl_T_cons(ctx, value, &nil);
                tail = &M_rest(*tail);
            }
            Fl_Gc_restore(ctx, gc);
            Fl_Gc_push(ctx, res);
        }
        return res;
    case '\'':
        if (!(value = Fl_read(ctx, rfn, data)))
            Fl_error(ctx, "unexpected quote (\"'\")");
        return Fl_T_cons(ctx, Fl_T_symbol(ctx, "quote"), Fl_T_cons(ctx, value, &nil));
    case '"':
        res = Fl_str_make(ctx, NULL, '\0');
        value = res;
        c = rfn(ctx, data);
        while (c != '"') {
            if (c == '\0')
                Fl_error(ctx, "string is missing a closing quote '\"'");
            if (c == '\\') { /* escape sequences */
                c = rfn(ctx, data);
                if (strchr("nrt", c))
                    c = strchr("n\nr\rt\t", c)[1];
            }
            value = Fl_str_make(ctx, value, c);
            c = rfn(ctx, data);
        }
        return res;
    default: {
        /* notice that there's a space here, it's also a valid delimiter */
        const char *const delimiters = "();\n\t\r ";
        char buf[MAX_BUF_LEN], *s = buf;
        do {
            if (s == &buf[sizeof(buf) - 1])
                Fl_error(ctx, "symbol is too long (do you really need THAT many character? ;))");
            *s++ = c;
            c = rfn(ctx, data);
        } while (c && !strchr(delimiters, c));
        *s = '\0';
        ctx->next_char = c;
        Fl_Number n = strtod(buf, &s); /* try to read as number */
        if (s != buf && strchr(delimiters, *s))
            return Fl_T_number(ctx, n);
        if (strcmp(buf, "nil") == 0)
            return &nil;
        return Fl_T_symbol(ctx, buf);
        }
    }
}

Fl_Object *Fl_read(Fl_Context *ctx, Fl_Read_fn fn, void *data) {
    Fl_Object *obj = p_read(ctx, fn, data);
    if (obj == &rpr)
        Fl_error(ctx, "unexpected parenthesis ')'");
    return obj;
}

static char p_readfp(Fl_Context *ctx, void *data) {
    M_unused(ctx);
    int c;
    return (c = fgetc(data)) != EOF ? c : '\0';
}

Fl_Object *Fl_readfp(Fl_Context *ctx, FILE *fp) {
    return Fl_read(ctx, p_readfp, fp);
}

static Fl_Object *p_eval(Fl_Context *ctx, Fl_Object *obj, Fl_Object *env, Fl_Object **bind);

static Fl_Object *p_eval_list(Fl_Context *ctx, Fl_Object *list, Fl_Object *env) {
    Fl_Object *res = &nil, **tail = &res;
    while (!M_isnil(list)) {
        *tail = Fl_T_cons(ctx, p_eval(ctx, Fl_next_arg(ctx, &list), env, NULL), &nil);
        tail = &M_rest(*tail);
    }
    return res;
}

static Fl_Object *do_list(Fl_Context *ctx, Fl_Object *list, Fl_Object *env) {
    Fl_Object *res = &nil;
    int save = Fl_Gc_save(ctx);
    while (!M_isnil(list)) {
        Fl_Gc_restore(ctx, save);
        Fl_Gc_push(ctx, list);
        Fl_Gc_push(ctx, env);
        res = p_eval(ctx, Fl_next_arg(ctx, &list), env, &env);
    }
    return res;
}

static Fl_Object *args_to_env(Fl_Context *ctx, Fl_Object *prm, Fl_Object *arg, Fl_Object *env) {
    while (!M_isnil(prm)) {
        if (M_type(prm) != T_PAIR) {
            env = Fl_T_cons(ctx, Fl_T_cons(ctx, prm, arg), env);
            break;
        }
        env = Fl_T_cons(ctx, Fl_T_cons(ctx, M_first(prm), Fl_first(ctx, arg)), env);
        prm = M_rest(prm);
        arg = Fl_rest(ctx, arg);
    }
    return env;
}

#define eval_arg() \
    p_eval(ctx, Fl_next_arg(ctx, &arg), env, NULL)

#define arithmetic_op(op) {                          \
        Fl_Number n = Fl_to_number(ctx, eval_arg()); \
        while (!M_isnil(arg))                        \
            n = n op Fl_to_number(ctx, eval_arg());  \
        res = Fl_T_number(ctx, n);                   \
    }

#define relational_op(op)                                       \
        val1 = p_check_type(ctx, eval_arg(), T_NUMBER);         \
        val2 = p_check_type(ctx, eval_arg(), T_NUMBER);         \
        res = Fl_T_bool(ctx, M_number(val1) op M_number(val2)); \

static Fl_Object *p_eval(Fl_Context *ctx, Fl_Object *obj, Fl_Object *env, Fl_Object **new_env) {
    Fl_Object cl, *val1, *val2;

    if (M_type(obj) == T_SYMBOL)
        return M_rest(p_get_bound(obj, env));
    if (M_type(obj) != T_PAIR)
        return obj;

    M_first(&cl) = obj;
    M_rest(&cl) = ctx->call_list;
    ctx->call_list = &cl;

    int gc = Fl_Gc_save(ctx);
    Fl_Object *fn = p_eval(ctx, M_first(obj), env, NULL),
        *arg = M_rest(obj),
        *res = &nil;

    switch (M_type(fn)) {
    case T_BUILTIN:
        switch (M_builtin(fn)) {
        case BI_LET:
            val1 = p_check_type(ctx, Fl_next_arg(ctx, &arg), T_SYMBOL);
            if (new_env)
                *new_env = Fl_T_cons(ctx, Fl_T_cons(ctx, val1, eval_arg()), env);
            break;
        case BI_SET:
            val1 = p_check_type(ctx, Fl_next_arg(ctx, &arg), T_SYMBOL);
            M_rest(p_get_bound(val1, env)) = eval_arg();
            break;
        case BI_IF:
            while (!M_isnil(arg)) {
                val1 = eval_arg();
                if (!M_isnil(val1)) {
                    res = M_isnil(arg) ? val1 : eval_arg();
                    break;
                }
                if (M_isnil(arg))
                    break;
                arg = M_rest(arg);
            }
            break;
        case BI_FN:
        case BI_MACRO:
            val1 = Fl_T_cons(ctx, env, arg);
            Fl_next_arg(ctx, &arg);
            res = Fl_object(ctx);
            M_settype(res, M_builtin(fn) == BI_FN ? T_FUNC : T_MACRO);
            M_rest(res) = val1;
            break;
        case BI_USE: {
            char file_name[MAX_BUF_LEN * 16]; /* 1KB, a pretty sensible buffer size */
            val1 = p_check_type(ctx, eval_arg(), T_STRING);
            Fl_to_string(ctx, val1, file_name, sizeof(file_name));

            if (*file_name == '@') { /* dynamic library */
                libload(ctx, file_name + 1, (char *[]){ "math_pow", "pow", "math_idiv", "//", "math_mod", "%" }, 6);
            } else {
                FILE *fp = fopen(file_name, "r");
                if (!fp)
                    Fl_error(ctx, "could not load file");
                Fl_run_file(ctx, fp);
            }
            break;
        }
        case BI_WHILE:
            val1 = Fl_next_arg(ctx, &arg);
            int n = Fl_Gc_save(ctx);
            while (!M_isnil(p_eval(ctx, val1, env, NULL))) {
                do_list(ctx, arg, env);
                Fl_Gc_restore(ctx, n);
            }
            break;
        case BI_QUOTE:
            res = Fl_next_arg(ctx, &arg);
            break;
        case BI_EVAL:
            val1 = p_check_type(ctx, Fl_next_arg(ctx, &arg), T_PAIR);
            res = do_list(ctx, val1, env);
            break;
        case BI_TYPE:
            res = Fl_T_string(ctx, types[Fl_type(ctx, eval_arg())]);
            break;
        case BI_AND:
            while (!M_isnil(arg) && !M_isnil(res = eval_arg())) {}
            break;
        case BI_OR:
            while (!M_isnil(arg) && M_isnil(res = eval_arg())) {}
            break;
        case BI_DO:
            res = do_list(ctx, arg, env);
            break;
        case BI_CONS:
            val1 = eval_arg();
            res = Fl_T_cons(ctx, val1, eval_arg());
            break;
        case BI_FIRST:
            res = Fl_first(ctx, eval_arg());
            break;
        case BI_REST:
            res = Fl_rest(ctx, eval_arg());
            break;
        case BI_SETF:
            val1 = p_check_type(ctx, eval_arg(), T_PAIR);
            M_first(val1) = eval_arg();
            break;
        case BI_SETR:
            val1 = p_check_type(ctx, eval_arg(), T_PAIR);
            M_rest(val1) = eval_arg();
            break;
        case BI_LIST:
            res = p_eval_list(ctx, arg, env);
            break;
        case BI_NOT:
            res = Fl_T_bool(ctx, M_isnil(eval_arg()));
            break;
        case BI_ATOM:
            res = Fl_T_bool(ctx, Fl_type(ctx, eval_arg()) != T_PAIR);
            break;
        case BI_PRINT:
            while (!M_isnil(arg)) {
                Fl_writefp(ctx, eval_arg(), stdout);
                if (!M_isnil(arg))
                    fputs(" ", stdout);
            }
            break;
        case BI_EQ:
            val1 = eval_arg();
            res = Fl_T_bool(ctx, p_equal(val1, eval_arg()));
            break;
        case BI_LT:
            relational_op(<);
            break;
        case BI_LE:
            relational_op(<=);
            break;
        case BI_GT:
            relational_op(>);
            break;
        case BI_GE:
            relational_op(>=);
            break;
        case BI_ADD:
            arithmetic_op(+);
            break;
        case BI_SUB:
            arithmetic_op(-);
            break;
        case BI_MUL:
            arithmetic_op(*);
            break;
        case BI_DIV:
            arithmetic_op(/);
            break;
        }
        break;
    case T_CFUNC:
        res = M_cfunc(fn)(ctx, p_eval_list(ctx, arg, env));
        break;
    case T_FUNC:
        arg = p_eval_list(ctx, arg, env);
        val1 = M_rest(fn);   /* (env params ...) */
        val2 = M_rest(val1); /* (params ...)    */
        res = do_list(ctx, M_rest(val2), args_to_env(ctx, M_first(val2), arg, M_first(val1)));
        break;
    case T_MACRO:
        val1 = M_rest(fn);   /* (env params ...) */
        val2 = M_rest(val1); /* (params ...)    */
        /* replace caller object with code generated by macro & re-evaluate */
        *obj = *do_list(ctx, M_rest(val2), args_to_env(ctx, M_first(val2), arg, M_first(val1)));
        Fl_Gc_restore(ctx, gc);
        ctx->call_list = M_rest(&cl);
        return p_eval(ctx, obj, env, NULL);
    default:
        Fl_error(ctx, "cannot call non-callable value");
    }
    Fl_Gc_restore(ctx, gc);
    Fl_Gc_push(ctx, res);
    ctx->call_list = M_rest(&cl);
    return res;
}

Fl_Object *Fl_eval(Fl_Context *ctx, Fl_Object *obj) {
    return p_eval(ctx, obj, &nil, NULL);
}

Fl_Context *Fl_open(void *ptr, int size) {
    /* initialize context struct */
    Fl_Context *ctx = ptr;
    memset(ctx, 0, sizeof(Fl_Context));
    ptr = (char *)ptr + sizeof(Fl_Context);
    size -= sizeof(Fl_Context);

    /* initialize objects memory region */
    ctx->objects = (Fl_Object *)ptr;
    ctx->nobject = size / sizeof(Fl_Object);
    /* initialize lists */
    ctx->call_list = ctx->free_list = ctx->sym_list = &nil;

    /* populate free list */
    for (int i = 0; i < ctx->nobject; ++i) {
        Fl_Object *obj = &ctx->objects[i];
        M_settype(obj, T_FREE);
        M_rest(obj) = ctx->free_list;
        ctx->free_list = obj;
    }

    /* initialize objects */
    ctx->t = Fl_T_symbol(ctx, "t");
    Fl_set(ctx, ctx->t, ctx->t);

    /* register built-ins */
    int save = Fl_Gc_save(ctx);
    for (int i = 0; i < BI_LEN; ++i) {
        Fl_Object *bi = Fl_object(ctx);
        M_settype(bi, T_BUILTIN);
        M_builtin(bi) = i;
        Fl_set(ctx, Fl_T_symbol(ctx, builtins[i]), bi);
        Fl_Gc_restore(ctx, save);
    }
    return ctx;
}

void Fl_close(Fl_Context *ctx) {
    /* clear gcstack and symbol list, which makes all objects unreachable */
    ctx->gcstack_index = 0;
    ctx->sym_list = &nil;
    Fl_Gc_collect(ctx);
}

void Fl_run_file(Fl_Context *ctx, FILE *fp) {
    int gc_index = Fl_Gc_save(ctx);
    Fl_Object *obj;
    while ((obj = Fl_readfp(ctx, fp))) {
        Fl_eval(ctx, obj);
        Fl_Gc_restore(ctx, gc_index);
    }
    fclose(fp);
}
