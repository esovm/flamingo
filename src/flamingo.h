#ifndef FLAMINGO_H
#define FLAMINGO_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define FLAMINGO_VERSION "0.6.0"
#define MAX_BUF_LEN 64 /* a random but reasonable number */

/* M_ prefix for all macro functions */

#define M_unused(P)        ((void)(P)) /* just tells the compiler to ignore parameter P */
#define M_isodd(X)         ((X) & 1)
#define M_first(X)         ((X)->car.o)
#define M_rest(X)          ((X)->cdr.o)
#define M_tag(X)           ((X)->car.c)
#define M_isnil(X)         ((X) == &nil)
#define M_type(X)          (M_isodd(M_tag(X)) ? M_tag(X) >> 2 : T_PAIR)
#define M_settype(X, T)    (M_tag(X) = (T) << 2 | 1)
#define M_number(X)        ((X)->cdr.n)
#define M_builtin(X)       ((X)->cdr.c)
#define M_cfunc(X)         ((X)->cdr.f)
#define M_strbuf(X)        (&(X)->car.c + 1)

#define STR_BUF_SIZE       ((int)sizeof(Fl_Object *) - 1)
#define GC_MARKBIT         2
#define GC_MAX_STACK_SIZE  256

typedef float Fl_Number;
typedef struct Fl_Object Fl_Object;
typedef struct Fl_Context Fl_Context;

typedef Fl_Object *(*Fl_CFunc)(Fl_Context *ctx, Fl_Object *args);
typedef void (*Fl_Error_fn)(Fl_Context *ctx, const char *err, Fl_Object *call_list);
typedef void (*Fl_Write_fn)(Fl_Context *ctx, void *data, char c);
typedef char (*Fl_Read_fn)(Fl_Context *ctx, void *data);

typedef struct {
    Fl_Error_fn error;
    Fl_CFunc mark, gc;
} Fl_Handlers;

typedef union {
    Fl_Object *o;
    Fl_CFunc f;
    Fl_Number n;
    char c;
} Fl_Value;

struct Fl_Object {
    Fl_Value car;
    Fl_Value cdr;
};

struct Fl_Context {
    Fl_Handlers handlers;
    Fl_Object *gcstack[GC_MAX_STACK_SIZE];
    int gcstack_index;
    Fl_Object *objects;
    int nobject; /* count */
    Fl_Object *call_list;
    Fl_Object *free_list;
    Fl_Object *sym_list;
    Fl_Object *t; /* everything that is not nil */
    int next_char;
};

enum {
    T_PAIR, T_FREE, T_NIL, T_NUMBER, T_SYMBOL,
    T_STRING, T_FUNC, T_MACRO, T_BUILTIN, T_CFUNC, T_PTR
};

/* nil symbol, acts as false, and as an empty list ("()") */
extern Fl_Object nil;

bool Fl_str_equal(Fl_Object *obj, const char *str);
Fl_Object *Fl_str_make(Fl_Context *ctx, Fl_Object *tail, int c);

Fl_Object *Fl_object(Fl_Context *ctx);

Fl_Context *Fl_open(void *ptr, int size);
void Fl_close(Fl_Context *ctx);
void Fl_run_file(Fl_Context *ctx, FILE *fp);
Fl_Handlers *Fl_handlers(Fl_Context *ctx);
void Fl_error(Fl_Context *ctx, const char *message);
Fl_Object *Fl_next_arg(Fl_Context *ctx, Fl_Object **arg);
int Fl_type(Fl_Context *ctx, Fl_Object *obj);
bool Fl_isnil(Fl_Context *ctx, Fl_Object *obj);

/* Mark and sweep garbage collecting */
void Fl_Gc_push(Fl_Context *ctx, Fl_Object *obj);
void Fl_Gc_restore(Fl_Context *ctx, int idx);
int Fl_Gc_save(Fl_Context *ctx);
void Fl_Gc_mark(Fl_Context *ctx, Fl_Object *obj);

Fl_Object *Fl_list(Fl_Context *ctx, Fl_Object **objects, int n);
Fl_Object *Fl_first(Fl_Context *ctx, Fl_Object *obj);
Fl_Object *Fl_rest(Fl_Context *ctx, Fl_Object *obj);

void Fl_write(Fl_Context *ctx, Fl_Object *obj, Fl_Write_fn fn, void *data, bool with_quotes);
void Fl_writefp(Fl_Context *ctx, Fl_Object *obj, FILE *fp);

int Fl_to_string(Fl_Context *ctx, Fl_Object *obj, char *buf, int size);
Fl_Number Fl_to_number(Fl_Context *ctx, Fl_Object *obj);
void *Fl_to_ptr(Fl_Context *ctx, Fl_Object *obj);

void Fl_set(Fl_Context *ctx, Fl_Object *sym, Fl_Object *v);
Fl_Object *Fl_read(Fl_Context *ctx, Fl_Read_fn fn, void *data);
Fl_Object *Fl_readfp(Fl_Context *ctx, FILE *fp);
Fl_Object *Fl_eval(Fl_Context *ctx, Fl_Object *obj);

#endif /* FLAMINGO_H */
