#ifndef FLAMINGO_GC_H
#define FLAMINGO_GC_H

#include "flamingo.h"

void Fl_Gc_push(Fl_Context *ctx, Fl_Object *obj);
void Fl_Gc_restore(Fl_Context *ctx, int index);
int Fl_Gc_save(Fl_Context *ctx);
void Fl_Gc_mark(Fl_Context *ctx, Fl_Object *obj);
void Fl_Gc_collect(Fl_Context *ctx);

#endif
