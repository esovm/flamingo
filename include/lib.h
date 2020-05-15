#ifndef FLAMINGO_LIB_H
#define FLAMINGO_LIB_H

#include "flamingo.h"

void bs_register_all(Fl_Context *);
void math_register_all(Fl_Context *);

void libload(Fl_Context *ctx, const char *libname, char **funcs, size_t len);

#endif /* FLAMINGO_LIB_H */
