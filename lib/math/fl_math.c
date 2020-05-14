#include <math.h>

#include "type.h"
#include "fl_math.h"

Fl_Object *math_pow(Fl_Context *ctx, Fl_Object *args) {
    Fl_Number a = Fl_to_number(ctx, Fl_next_arg(ctx, &args));
    Fl_Number b = Fl_to_number(ctx, Fl_next_arg(ctx, &args));
    return Fl_T_number(ctx, pow(a, b));
}

Fl_Object *math_idiv(Fl_Context *ctx, Fl_Object *args) {
    Fl_Number a = Fl_to_number(ctx, Fl_next_arg(ctx, &args));
    Fl_Number b = Fl_to_number(ctx, Fl_next_arg(ctx, &args));
    return Fl_T_number(ctx, (int)a / (int)b);
}

Fl_Object *math_mod(Fl_Context *ctx, Fl_Object *args) {
    Fl_Number a = Fl_to_number(ctx, Fl_next_arg(ctx, &args));
    Fl_Number b = Fl_to_number(ctx, Fl_next_arg(ctx, &args));
    return Fl_T_number(ctx, fmod(a, b));
}

// Fl_set(ctx, Fl_T_symbol(ctx, "pi"), Fl_T_number(ctx, 3.141592653589793238462643383279502884));
// Fl_set(ctx, Fl_T_symbol(ctx, "e"), Fl_T_number(ctx, 2.718281828459045235360287471352662497));
