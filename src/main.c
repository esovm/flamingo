#include <setjmp.h>

#include "flamingo.h"

static jmp_buf global_execution_context;
static char buf[1024 * 64];

static void onerror(Fl_Context *ctx, const char *message, Fl_Object *call_list)
{
    M_unused(ctx);
    M_unused(call_list);
    fprintf(stderr, "[error] %s\n", message);
    longjmp(global_execution_context, -1);
}

int main(int argc, char **argv)
{
    Fl_Object *obj;
    FILE *volatile fp = stdin;
    Fl_Context *ctx = Fl_open(buf, sizeof(buf));

    if (argc > 1) {
        if (!(fp = fopen(argv[1], "r")))
            Fl_error(ctx, "Couldn't open file");
    }
    if (fp == stdin) {
        Fl_handlers(ctx)->error = onerror;
        printf("Flamingo %s\n", FLAMINGO_VERSION);
    }

    int gc = Fl_Gc_save(ctx);
    setjmp(global_execution_context);

    while (true) {
        Fl_Gc_restore(ctx, gc);
        if (fp == stdin)
            fputs("=> ", stdout);
        if (!(obj = Fl_readfp(ctx, fp)))
            break;
        obj = Fl_eval(ctx, obj);
        if (fp == stdin) {
            Fl_writefp(ctx, obj, stdout);
            putchar('\n');
        }
    }
    return 0;
}
