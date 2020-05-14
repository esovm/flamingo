#ifndef FLAMINGO_LIB_H
#define FLAMINGO_LIB_H

#include "flamingo.h"

void bs_register_all(Fl_Context *);
void math_register_all(Fl_Context *);

int libload(Fl_Context *ctx, const char *name);

#endif /* FLAMINGO_LIB_H */
