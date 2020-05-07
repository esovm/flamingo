#include <string.h>
#include <setjmp.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include "lib.h"
#include "util.h"
#include "flamingo.h"

static jmp_buf global_execution_context;
static char buf[1024 * 64];

static void p_error(Fl_Context *ctx, const char *message, Fl_Object *call_list) {
    M_unused(ctx);
    M_unused(call_list);
    fprintf(stderr, "[error] %s\n", message);
    longjmp(global_execution_context, -1);
}

static void p_load(Fl_Context *ctx, const char *fn) {
    FILE *fp;
    if ((fp = fopen(fn, "r"))) {
        Fl_run_file(ctx, fp);
    } else {
        const char *homedir = getenv("HOME");
        /* 16 is strlen of "/.flamingo/lib/" */
        char *full_path = malloc(strlen(homedir) + strlen(fn) + 16);
        if (!homedir) /* HOME is not set */
            homedir = getpwuid(getuid())->pw_dir;
        if (!full_path) {
            fputs("malloc failure...\n", stderr);
            exit(1);
        }
        strcpy(full_path, homedir);
        strcat(full_path, "/.flamingo/lib/");
        strcat(full_path, fn);
        if (!(fp = fopen(full_path, "r")))
            Fl_error(ctx, "could not load library");
        free(full_path);
        Fl_run_file(ctx, fp);
    }
}

int main(int argc, char **argv) {
    Fl_Object *obj;
    FILE *volatile fp = stdin;
    Fl_Context *ctx = Fl_open(buf, sizeof(buf));
    char *exec_str = NULL;
    int c;

    p_load(ctx, "base.fl");
    bs_register_all(ctx);

    while ((c = getopt(argc, argv, "s:")) != -1) {
        switch (c) {
        case 's':
            exec_str = optarg;
            break;
        case '?':
            if (optopt == 's')
                fprintf(stderr, "Option -%c requires an argument\n", optopt);
            else
                fprintf(stderr, "Unknown option -%c\n", optopt);
            /* fallthrough */
        default:
            return 1;
        }
    }
    if (exec_str) {
        if (!(fp = tmpfile())) {
            fputs("unexpected error, could not execute given string\n", stderr);
            return 1;
        }
        fputs(exec_str, fp);
    } else if (argc > 1 && !(fp = fopen(argv[1], "r"))) {
        Fl_error(ctx, "could not open file");
    }

    if (fp == stdin) {
        Fl_handlers(ctx)->error = p_error;
        printf("Flamingo %s on %s\n", FLAMINGO_VERSION, os_name());
    }

    int gci = Fl_Gc_save(ctx); /* gc stack index */
    setjmp(global_execution_context);

    while (true) {
        if (fp == stdin)
            fputs("=> ", stdout);
        if (!(obj = Fl_readfp(ctx, fp)))
            break;
        obj = Fl_eval(ctx, obj);
        if (fp == stdin || exec_str) {
            Fl_writefp(ctx, obj, stdout);
            putchar('\n');
        }
        Fl_Gc_restore(ctx, gci);
    }
    fclose(fp);
    return 0;
}
