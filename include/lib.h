#ifndef FLAMINGO_LIB_H
#define FLAMINGO_LIB_H

#include "flamingo.h"

extern char *base_lib[];
extern char *math_lib[];

void dl_load(Fl_Context *ctx, const char *libname, char **funcs);

#endif /* FLAMINGO_LIB_H */
