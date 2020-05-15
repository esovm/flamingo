#include <dlfcn.h>

#include "lib.h"
#include "type.h"

void libload(Fl_Context *ctx, const char *libname, char **funcs) {
    void *handle = dlopen(libname, RTLD_LAZY);
    char *err;

    if (!handle)
        Fl_error(ctx, "could not load shared object");

    dlerror(); /* clear any existing error */

    for (size_t i = 0; funcs[i]; i += 2) {
        Fl_CFunc sym;
        *(void **)(&sym) = dlsym(handle, funcs[i]);
        if ((err = dlerror())) {
            fputs(err, stderr);
            return;
        }
        Fl_set(ctx, Fl_T_symbol(ctx, funcs[i + 1]), Fl_T_cfunc(ctx, sym));
    }
}
