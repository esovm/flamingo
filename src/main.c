#include <string.h>
#include <setjmp.h>
#include <unistd.h>

#include "lib.h"
#include "util.h"
#include "config.h"
#include "flamingo.h"

static jmp_buf global_execution_context;
static char buf[1024 * 64];

__attribute__((noreturn)) static void p_print_help(int exit_status, char **av) {
    printf("%s\n"
    "Usage: %s [-hv] [-s string] [file ...]\n"
    "Options:\n"
    "  -s str   execute string 'str'\n"
    "  -h       print help (this text) and exit\n"
    "  -v       print version information and exit\n", FL_HELP_HEADER, *av);
    exit(exit_status);
}

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
        const char *homedir = get_home();
        char *full_path = malloc(strlen(homedir) + strlen(FL_PATH) + strlen(fn));
        if (!full_path) {
            fputs("malloc failure...\n", stderr);
            exit(EXIT_FAILURE);
        }
        strcpy(full_path, homedir);
        strcat(full_path, FL_PATH);
        strcat(full_path, "/lib/");
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

    while ((c = getopt(argc, argv, "vhs:")) != -1) {
        switch (c) {
        case 'v':
            printf("%s %s\nCopyright (C) 2020 Tomer Shechner\n", FL_PROGRAM_NAME, FL_VERSION);
            return EXIT_SUCCESS;
        case 'h':
            p_print_help(EXIT_SUCCESS, argv);
            break;
        case 's':
            exec_str = optarg;
            break;
        default:
            p_print_help(EXIT_FAILURE, argv);
        }
    }

    if (exec_str) {
        if (!(fp = tmpfile())) {
            fprintf(stderr, "unexpected error, could not execute given string '%s'\n", exec_str);
            return EXIT_FAILURE;
        }
        fputs(exec_str, fp);
    } else if (argc > 1 && !(fp = fopen(argv[1], "r"))) {
        Fl_error(ctx, "could not open file");
    }

    if (fp == stdin) {
        Fl_handlers(ctx)->error = p_error;
        printf("%s %s on %s\n", FL_PROGRAM_NAME, FL_VERSION, os_name());
    }

    int gci = Fl_Gc_save(ctx); /* gc stack index */
    setjmp(global_execution_context);

    while (true) {
        if (fp == stdin)
            fputs("=> ", stdout);
        if (!(obj = Fl_readfp(ctx, fp)))
            break;
        obj = Fl_eval(ctx, obj);
        if (exec_str || fp == stdin) {
            Fl_writefp(ctx, obj, stdout);
            putchar('\n');
        }
        Fl_Gc_restore(ctx, gci);
    }
    fclose(fp);
    return EXIT_SUCCESS;
}
