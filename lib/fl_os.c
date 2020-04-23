#include "type.h"
#include "fl_os.h"

static Fl_Object *os_platform(Fl_Context *ctx, Fl_Object *args)
{
    M_unused(args);
#ifdef _WIN32
    return Fl_T_string(ctx, "Windows");
#elif __APPLE__ || __MACH__
    return Fl_T_string(ctx, "macOS");
#elif __linux__
    return Fl_T_string(ctx, "Linux");
#elif __FreeBSD__
    return Fl_T_string(ctx, "FreeBSD");
#elif __unix__ || __unix
    return Fl_T_string(ctx, "Unix");
#else
    return Fl_T_string(ctx, "Unknown");
#endif
}

void os_register_all(Fl_Context *ctx)
{
    Fl_set(ctx, Fl_T_symbol(ctx, "platform"), Fl_T_cfunc(ctx, os_platform));
}
