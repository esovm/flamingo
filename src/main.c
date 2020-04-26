#include <setjmp.h>

#include "fl_lib.h"
#include "flamingo.h"

static jmp_buf global_execution_context;
static char buf[1024 * 64];

static void p_error(Fl_Context *ctx, const char *message, Fl_Object *call_list) {
    M_unused(ctx);
    M_unused(call_list);
    fprintf(stderr, "[error] %s\n", message);
    longjmp(global_execution_context, -1);
}

int main(int argc, char **argv) {
    Fl_Object *obj;
    FILE *base, *volatile fp = stdin;
    Fl_Context *ctx = Fl_open(buf, sizeof(buf));

    if (!(base = fopen("lib/base.fl", "r")))
        Fl_error(ctx, "Could not load base library");
    Fl_run_file(ctx, base);

    aux_register_all(ctx);

    if (argc > 1 && !(fp = fopen(argv[1], "r")))
        Fl_error(ctx, "Could not open file");
    if (fp == stdin) {
        Fl_handlers(ctx)->error = p_error;
        printf("Flamingo %s\n", FLAMINGO_VERSION);
    }

    int gci = Fl_Gc_save(ctx); /* gc stack index */
    setjmp(global_execution_context);

    while (true) {
        if (fp == stdin)
            fputs("=> ", stdout);
        if (!(obj = Fl_readfp(ctx, fp)))
            break;
        obj = Fl_eval(ctx, obj);
        if (fp == stdin) {
            Fl_writefp(ctx, obj, stdout);
            putchar('\n');
        }
        Fl_Gc_restore(ctx, gci);
    }
    fclose(fp);
    return 0;
}
