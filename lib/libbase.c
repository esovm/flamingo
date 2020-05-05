/* Auxiliary functions */

#define  _GNU_SOURCE

#include <errno.h>

#include "type.h"
#include "util.h"
#include "lib.h"

static Fl_Object *bs_platform(Fl_Context *ctx, Fl_Object *args) {
    M_unused(args);
    return Fl_T_string(ctx, os_name());
}

static Fl_Object *bs_read(Fl_Context *ctx, Fl_Object *args) {
    M_unused(args);
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;
    if ((nread = getline(&line, &len, stdin)) == -1)
        return Fl_T_bool(ctx, false);
    if (line[nread - 1] == '\n')
        line[nread-- - 1] = '\0';
    Fl_Object *ret = Fl_T_string(ctx, line);
    free(line);
    return ret;
}

static Fl_Object *bs_str_to_num(Fl_Context *ctx, Fl_Object *args) {
    errno = 0;
    char *end, buf[MAX_BUF_LEN];
    Fl_to_string(ctx, Fl_next_arg(ctx, &args), buf, sizeof(buf));
    Fl_Number n = strtod(buf, &end);
    return Fl_T_number(ctx, *end ? -1 : n);
}

void bs_register_all(Fl_Context *ctx) {
    Fl_set(ctx, Fl_T_symbol(ctx, "platform"), Fl_T_cfunc(ctx, bs_platform));
    Fl_set(ctx, Fl_T_symbol(ctx, "read"), Fl_T_cfunc(ctx, bs_read));
    Fl_set(ctx, Fl_T_symbol(ctx, "num"), Fl_T_cfunc(ctx, bs_str_to_num));
}
