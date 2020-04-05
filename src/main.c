#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>

#include "mpc.h"
#include "util.h"
#include "value.h"

#define GG_VERSION "0.1.0"

int main(int argc, char **argv)
{
	char *line, *grammar = readfile("src/grammar");

	mpc_parser_T *num = mpc_new("number");
	mpc_parser_T *sym = mpc_new("symbol");
	mpc_parser_T *sexpr = mpc_new("sexpression");
	mpc_parser_T *qexpr = mpc_new("qexpression");
	mpc_parser_T *expr = mpc_new("expression");
	mpc_parser_T *gg = mpc_new("gg");

	mpca_lang(MPCA_LANG_DEFAULT, grammar, num, sym, sexpr, qexpr, expr, gg);

	free(grammar);

	printf("GG %s\ntype \"quit\" to terminate\n", GG_VERSION);

	while ((line = readline("=> "))) {
		if (strcmpws(line, "quit"))
			break;
		if (!*line) {
			free(line);
			continue;
		}

		mpc_result_T res;
		if (mpc_parse("<stdin>", line, gg, &res)) {
			Value *v = value_eval(value_read(res.output));
			value_dump(v);
			putchar('\n');
			value_free(v);
			mpc_ast_delete(res.output);
		} else {
			mpc_err_print(res.error);
			mpc_err_delete(res.error);
		}

		add_history(line);
		free(line);
	}

	mpc_cleanup(6, num, sym, sexpr, qexpr, expr, gg);

	return 0;
}
