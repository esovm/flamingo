#include <math.h>

#include "type.h"
#include "lib.h"

Fl_Object *_pow(Fl_Context *ctx, Fl_Object *args) {
    Fl_Number a = Fl_to_number(ctx, Fl_next_arg(ctx, &args));
    Fl_Number b = Fl_to_number(ctx, Fl_next_arg(ctx, &args));
    return Fl_T_number(ctx, pow(a, b));
}

Fl_Object *_idiv(Fl_Context *ctx, Fl_Object *args) {
    Fl_Number a = Fl_to_number(ctx, Fl_next_arg(ctx, &args));
    Fl_Number b = Fl_to_number(ctx, Fl_next_arg(ctx, &args));
    return Fl_T_number(ctx, (int)a / (int)b);
}

Fl_Object *_mod(Fl_Context *ctx, Fl_Object *args) {
    Fl_Number a = Fl_to_number(ctx, Fl_next_arg(ctx, &args));
    Fl_Number b = Fl_to_number(ctx, Fl_next_arg(ctx, &args));
    return Fl_T_number(ctx, fmod(a, b));
}

char *math_lib[] = { "_pow", "pow", "_idiv", "//", "_mod", "%", NULL };
