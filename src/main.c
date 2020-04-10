#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>

#include "mpc.h"
#include "util.h"
#include "object.h"
#include "env.h"

#define CRANE_VERSION "0.1.0"

mpc_parser_T *com, *num, *sym, *str, *sexpr, *bexpr, *expr, *crane;

static void repl(Env *env, mpc_parser_T *par)
{
    mpc_result_T res;
    char *line;

    printf("Crane %s\ntype \"exit\" to terminate\n", CRANE_VERSION);

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
    num = mpc_new("number");
    sym = mpc_new("symbol");
    str = mpc_new("string");
    sexpr = mpc_new("sexpression");
    bexpr = mpc_new("bexpression");
    expr = mpc_new("expression");
    crane = mpc_new("crane");

    mpca_lang(MPCA_LANG_DEFAULT,
        "comment      : /#[^\\r\\n]*/;"
        "number       : /-?\\.?\\d+\\.?\\d*/;"
        "symbol       : /[a-zA-Z0-9_+\\-*\\/%^\\\\=<>!&@\\|~$]+/;"
        "string       : /'(\\\\.|[^'])*'/;"
        "sexpression  : '(' <expression>* ')';"
        "bexpression  : '[' <expression>* ']';"
        "expression   : <comment> | <number> | <symbol> | <string> | <sexpression> | <bexpression>;"
        "crane           : /^/ <expression>* /$/;", com, num, sym, str, sexpr, bexpr, expr, crane);

    if (argc == 1) {
        repl(env, crane);
    } else {
        for (int i = 1; i < argc; ++i) {
            Object *r, *args = obj_append(obj_new_sexpr(), obj_new_str(argv[i]));
            if ((r = bi_use(env, args))->type == O_ERROR)
                obj_dump(r);
            obj_free(r);
        }
    }

    env_free(env);
    mpc_cleanup(8, com, num, sym, str, sexpr, bexpr, expr, crane);

    return 0;
}
