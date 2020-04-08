#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>

#include "mpc.h"
#include "util.h"
#include "object.h"
#include "env.h"

#define GG_VERSION "0.1.0"

static void repl(Env *env, const mpc_parser_T *par)
{
	mpc_result_T res;
	char *line;

	printf("GG %s\ntype \"exit\" to terminate\n", GG_VERSION);

	while ((line = readline("=> "))) {
		if (!*line) {
			free(line);
			continue;
		}

		if (mpc_parse("<stdin>", line, (mpc_parser_T *)par, &res)) {
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
}

int main(int argc, char **argv)
{
	Env *env = env_new();
	const mpc_parser_T
		*num = mpc_new("number"),
		*sym = mpc_new("symbol"),
		*sexpr = mpc_new("sexpression"),
		*bexpr = mpc_new("bexpression"),
		*expr = mpc_new("expression"),
		*gg = mpc_new("gg");

	mpca_lang(MPCA_LANG_DEFAULT,
		"number       : /-?\\.?\\d+\\.?\\d*/;"
		"symbol       : /[a-zA-Z0-9_+\\-*\\/%^\\\\=<>!&@\\|~]+/;"
		"sexpression  : '(' <expression>* ')';"
		"bexpression  : '[' <expression>* ']';"
		"expression   : <number> | <symbol> | <sexpression> | <bexpression>;"
		"gg           : /^/ <expression>* /$/;", num, sym, sexpr, bexpr, expr, gg);

	repl(env, gg);

	env_free(env);

	mpc_cleanup(6, num, sym, sexpr, bexpr, expr, gg);

	return 0;
}
