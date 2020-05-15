#include "gc.h"

void Fl_Gc_push(Fl_Context *ctx, Fl_Object *obj) {
    if (ctx->gcstack_index == GC_MAX_STACK_SIZE)
        Fl_error(ctx, "garbage collector stack overflow :(");
    ctx->gcstack[ctx->gcstack_index++] = obj;
}

void Fl_Gc_restore(Fl_Context *ctx, int index) {
    ctx->gcstack_index = index;
}

int Fl_Gc_save(Fl_Context *ctx) {
    return ctx->gcstack_index;
}

void Fl_Gc_mark(Fl_Context *ctx, Fl_Object *obj) {
begin:
    if (M_tag(obj) & GC_MARKBIT)
        return;
    Fl_Object *car = M_first(obj); /* store car before modifying it with GC_MARKBIT */
    M_tag(obj) |= GC_MARKBIT;

    switch (M_type(obj)) {
    case T_PAIR:
        Fl_Gc_mark(ctx, car);
        /* FALLTHROUGH */
    case T_FUNC:
    case T_MACRO:
    case T_SYMBOL:
    case T_STRING:
        obj = M_rest(obj);
        goto begin;
    case T_PTR:
        if (ctx->handlers.mark)
            ctx->handlers.mark(ctx, obj);
        break;
    }
}

void Fl_Gc_collect(Fl_Context *ctx) {
    /* mark all */
    for (int i = 0; i < ctx->gcstack_index; ++i)
        Fl_Gc_mark(ctx, ctx->gcstack[i]);
    Fl_Gc_mark(ctx, ctx->sym_list);
    /* sweep and unmark */
    for (int i = 0; i < ctx->nobject; ++i) {
        Fl_Object *obj = &ctx->objects[i];
        if (M_type(obj) == T_FREE)
            continue; /* nothing to do, move on */
        if (~M_tag(obj) & GC_MARKBIT) {
            if (M_type(obj) == T_PTR && ctx->handlers.gc)
                ctx->handlers.gc(ctx, obj);
            M_settype(obj, T_FREE);
            M_rest(obj) = ctx->free_list;
            ctx->free_list = obj;
        } else {
            M_tag(obj) &= ~GC_MARKBIT;
        }
    }
}
