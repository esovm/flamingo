#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>

#include "mpc.h"
#include "util.h"
#include "object.h"
#include "env.h"

#define FLAMINGO_VERSION "0.1.0"

mpc_parser_T *com, *boolean, *num, *sym, *str, *sexpr, *bexpr, *expr, *flamingo;

static void repl(Env *env, mpc_parser_T *par)
{
    mpc_result_T res;
    char *line;

    printf("Flamingo %s\ntype \"exit\" to terminate\n", FLAMINGO_VERSION);

    while ((line = readline("=> "))) {
        if (!*line) {
            free(line);
            continue;
        }

        if (mpc_parse("<stdin>", line, par, &res)) {
            Object *obj = obj_eval(env, obj_read(res.output));
            obj_dump(obj);
            putchar('\n');
            obj_free(obj);
            mpc_ast_delete(res.output);
        } else {
            mpc_err_print(res.error);
            mpc_err_delete(res.error);
        }

        add_history(line);
        free(line);
    }
    putchar('\n');
}

int main(int argc, char **argv)
{
    Env *env = env_new();
    com = mpc_new("comment");
    boolean = mpc_new("boolean");
    num = mpc_new("number");
    sym = mpc_new("symbol");
    str = mpc_new("string");
    sexpr = mpc_new("sexpression");
    bexpr = mpc_new("bexpression");
    expr = mpc_new("expression");
    flamingo = mpc_new("flamingo");

    mpca_lang(MPCA_LANG_DEFAULT,
        "comment      : /#[^\\r\\n]*/;"
        "boolean      : \"true\" | \"false\";"
        "number       : /-?\\.?\\d+\\.?\\d*/;"
        "symbol       : /[a-zA-Z0-9_+\\-*\\/%^\\\\=<>!&@\\|~$]+/;"
        "string       : /'(\\\\.|[^'])*'/;"
        "sexpression  : '(' <expression>* ')';"
        "bexpression  : '[' <expression>* ']';"
        "expression   : <comment> | <boolean> | <number> | <symbol>"
        "| <string> | <sexpression> | <bexpression>;"
        "flamingo        : /^/ <expression>* /$/;", com, boolean, num, sym, str, sexpr, bexpr, expr, flamingo);

    if (argc == 1) {
        repl(env, flamingo);
    } else {
        for (int i = 1; i < argc; ++i) {
            Object *r, *args = obj_append(obj_new_sexpr(), obj_new_str(argv[i]));
            if ((r = bi_use(env, args))->type == O_ERROR)
                obj_dump(r);
            obj_free(r);
        }
    }

    env_free(env);
    mpc_cleanup(9, com, boolean, num, sym, str, sexpr, bexpr, expr, flamingo);

    return 0;
}
