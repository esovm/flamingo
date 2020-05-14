/* Auxiliary functions */

#define _GNU_SOURCE

#include <errno.h>
#include <float.h>
#include <ctype.h>

#include "type.h"
#include "util.h"
#include "lib.h"

typedef enum {
    S2N_SUCCESS       =  0,
    S2N_OVERFLOW      = -1,
    S2N_UNDERFLOW     = -2,
    S2N_INCONVERTIBLE = -3,
} str2num_errno;

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

static str2num_errno str2num(Fl_Number *out, char *s) {
    char *end;
    if (!*s || isspace((unsigned char)*s))
        return S2N_INCONVERTIBLE;
    errno = 0;
    Fl_Number n = strtod(s, &end);
    /* both checks are needed because DBL_MAX == LDBL_MAX is possible */
    if (n > DBL_MAX || (errno == ERANGE && n == LDBL_MAX))
        return S2N_OVERFLOW;
    if (n < DBL_MIN || (errno == ERANGE && n == LDBL_MIN))
        return S2N_UNDERFLOW;
    if (*end)
        return S2N_INCONVERTIBLE;
    *out = n;
    return S2N_SUCCESS;
}

static Fl_Object *bs_str_to_num(Fl_Context *ctx, Fl_Object *args) {
    char buf[MAX_BUF_LEN];
    Fl_to_string(ctx, Fl_next_arg(ctx, &args), buf, sizeof(buf));
    Fl_Number n;
    return str2num(&n, strip(buf)) == S2N_SUCCESS ? Fl_T_number(ctx, n) : Fl_T_bool(ctx, false);
}

void bs_register_all(Fl_Context *ctx) {
    Fl_set(ctx, Fl_T_symbol(ctx, "platform"), Fl_T_cfunc(ctx, bs_platform));
    Fl_set(ctx, Fl_T_symbol(ctx, "read"), Fl_T_cfunc(ctx, bs_read));
    Fl_set(ctx, Fl_T_symbol(ctx, "num"), Fl_T_cfunc(ctx, bs_str_to_num));
}
