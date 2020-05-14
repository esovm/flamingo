#include <dlfcn.h>

#include "lib.h"
#include "type.h"

int libload(Fl_Context *ctx, const char *name) {
    void *handle = dlopen(name, RTLD_LAZY);
    const char *c_names[] = { "math_pow", "math_idiv", "math_mod" };
    const char *fl_names[] = { "pow", "//", "%" };

    if (!handle)
        return -1;

    // for (size_t i = 0; i < sizeof(c_names) / sizeof(*c_names); ++i) {
    //     Fl_CFunc sym;
    //     *(void **)(&sym) = dlsym(handle, c_names[i]);
    //     if (!sym)
    //         continue;
    //     Fl_set(ctx, Fl_T_symbol(ctx, fl_names[i]), Fl_T_cfunc(ctx, sym));
    // }

    Fl_CFunc sym;
    *(void **)(&sym) = dlsym(handle, c_names[0]);
    Fl_set(ctx, Fl_T_symbol(ctx, fl_names[0]), Fl_T_cfunc(ctx, sym));

    dlclose(handle);
    return 0;
}
