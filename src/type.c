#include <string.h>

#include "type.h"

Fl_Object *Fl_str_make(Fl_Context *ctx, Fl_Object *tail, int c) {
    if (!tail || M_strbuf(tail)[STR_BUF_SIZE - 1]) {
        Fl_Object *obj = Fl_T_cons(ctx, NULL, &nil);
        M_settype(obj, T_STRING);
        if (tail) {
            M_rest(tail) = obj;
            --ctx->gcstack_index;
        }
        tail = obj;
    }
    M_strbuf(tail)[strlen(M_strbuf(tail))] = c;
    return tail;
}

Fl_Object *Fl_T_cons(Fl_Context *ctx, Fl_Object *car, Fl_Object *cdr) {
    Fl_Object *obj = Fl_object(ctx);
    M_first(obj) = car;
    M_rest(obj) = cdr;
    return obj;
}

Fl_Object *Fl_T_bool(Fl_Context *ctx, bool b) {
    return b ? ctx->t : &nil;
}

Fl_Object *Fl_T_number(Fl_Context *ctx, Fl_Number n) {
    Fl_Object *obj = Fl_object(ctx);
    M_settype(obj, T_NUMBER);
    M_number(obj) = n;
    return obj;
}

Fl_Object *Fl_T_string(Fl_Context *ctx, const char *str) {
    Fl_Object *obj = Fl_str_make(ctx, NULL, '\0'),
        *tail = obj;
    while (*str)
        tail = Fl_str_make(ctx, tail, *str++);
    return obj;
}

Fl_Object *Fl_T_symbol(Fl_Context *ctx, const char *name) {
    /* try to find it in the symbol list */
    for (Fl_Object *obj = ctx->sym_list; !M_isnil(obj); obj = M_rest(obj))
        if (Fl_str_equal(M_first(M_rest(M_first(obj))), name))
            return M_first(obj);
    /* wasn't found, create a new object, push to symbol list and return */
    Fl_Object *new_obj = Fl_object(ctx);
    M_settype(new_obj, T_SYMBOL);
    M_rest(new_obj) = Fl_T_cons(ctx, Fl_T_string(ctx, name), &nil);
    ctx->sym_list = Fl_T_cons(ctx, new_obj, ctx->sym_list);
    return new_obj;
}

Fl_Object *Fl_T_cfunc(Fl_Context *ctx, Fl_CFunc fn) {
    Fl_Object *obj = Fl_object(ctx);
    M_settype(obj, T_CFUNC);
    M_cfunc(obj) = fn;
    return obj;
}

Fl_Object *Fl_T_ptr(Fl_Context *ctx, void *ptr) {
    Fl_Object *obj = Fl_object(ctx);
    M_settype(obj, T_PTR);
    M_rest(obj) = ptr;
    return obj;
}
