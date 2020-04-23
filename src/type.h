#ifndef FLAMINGO_TYPE_H
#define FLAMINGO_TYPE_H

#include "flamingo.h"

Fl_Object *Fl_T_cons(Fl_Context *ctx, Fl_Object *car, Fl_Object *cdr);
Fl_Object *Fl_T_bool(Fl_Context *ctx, bool b);
Fl_Object *Fl_T_number(Fl_Context *ctx, Fl_Number n);
Fl_Object *Fl_T_string(Fl_Context *ctx, const char *str);
Fl_Object *Fl_T_symbol(Fl_Context *ctx, const char *name);
Fl_Object *Fl_T_cfunc(Fl_Context *ctx, Fl_CFunc fn);
Fl_Object *Fl_T_ptr(Fl_Context *ctx, void *ptr);

#endif
