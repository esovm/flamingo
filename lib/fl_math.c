#include <math.h>

#include "type.h"
#include "fl_math.h"

static Fl_Object *math_pow(Fl_Context *ctx, Fl_Object *args)
{
    Fl_Number a = Fl_to_number(ctx, Fl_next_arg(ctx, &args));
    Fl_Number b = Fl_to_number(ctx, Fl_next_arg(ctx, &args));
    return Fl_T_number(ctx, pow(a, b));
}

static Fl_Object *math_idiv(Fl_Context *ctx, Fl_Object *args)
{
    Fl_Number a = Fl_to_number(ctx, Fl_next_arg(ctx, &args));
    Fl_Number b = Fl_to_number(ctx, Fl_next_arg(ctx, &args));
    return Fl_T_number(ctx, (int)a / (int)b);
}

void math_register_all(Fl_Context *ctx)
{
    Fl_set(ctx, Fl_T_symbol(ctx, "pow"), Fl_T_cfunc(ctx, math_pow));
    Fl_set(ctx, Fl_T_symbol(ctx, "//"), Fl_T_cfunc(ctx, math_idiv));
}
