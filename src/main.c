#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>

#include "mpc.h"
#include "util.h"

#define VERSION "0.1.0"

double evalop(int a, const char *op, int b)
{
	switch (*op) {
	case '+': return a + b;
	case '-': return a - b;
	case '*': return a * b;
	case '/': return a / b;
	case '%': return a % b;
	case '^': return pow(a, b);
	}

	if (strcmp(op, "min") == 0) return U_MIN(a, b);
	if (strcmp(op, "max") == 0) return U_MAX(a, b);

	return 0;
}

double eval(mpc_ast_T *ast)
{
	if (strstr(ast->tag, "number")) {
		double n;
		if (str2dbl(&n, ast->contents) == S2D_SUCCESS)
			return n;
	}

	char *op = ast->children[1]->contents;
	double r = eval(ast->children[2]);
	int i = 3;

	while (strstr(ast->children[i]->tag, "expr"))
		r = evalop(r, op, eval(ast->children[i++]));

	return r;
}

static void print_license(void)
{
	fputs("This is free and unencumbered software released into the public domain.\n\n"
		"Anyone is free to copy, modify, publish, use, compile, sell, or\n"
		"distribute this software, either in source code form or as a compiled\n"
		"binary, for any purpose, commercial or non-commercial, and by any\n"
		"means.\n\n"
		"In jurisdictions that recognize copyright laws, the author or authors\n"
		"of this software dedicate any and all copyright interest in the\n"
		"software to the public domain. We make this dedication for the benefit\n"
		"of the public at large and to the detriment of our heirs and\n"
		"successors. We intend this dedication to be an overt act of\n"
		"relinquishment in perpetuity of all present and future rights to this\n"
		"software under copyright law.\n\n"
		"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND,\n"
		"EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF\n"
		"MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.\n"
		"IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR\n"
		"OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,\n"
		"ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR\n"
		"OTHER DEALINGS IN THE SOFTWARE.\n\n"
		"For more information, please refer to <http://unlicense.org/>\n", stdout);
}

int main(int argc, char **argv)
{
	char *line;

	mpc_parser_T *num = mpc_new("number");
	mpc_parser_T *op = mpc_new("operator");
	mpc_parser_T *expr = mpc_new("expression");
	mpc_parser_T *prog = mpc_new("program");

	mpca_lang(MPCA_LANG_DEFAULT,
		"number      : /\\d+\\.?\\d*/;"
		"operator    : '+' | '-' | '*' | '/' | '%' | '^' | \"max\" | \"min\";"
		"expression  : <number> | '(' <operator> <expression>+ ')';"
		"program     : /^/ <operator> <expression>+ /$/;",
		num, op, expr, prog);

	printf("GG %s\ntype \"quit\" to terminate, \"license\" for license text\n", VERSION);

	while ((line = readline("=> "))) {
		if (strcmpws(line, "quit"))
			break;
		if (strcmpws(line, "license")) {
			print_license();
			goto done;
		}
		if (!*line) {
			free(line);
			continue;
		}

		mpc_result_T res;
		if (mpc_parse("<stdin>", line, prog, &res)) {
			printf("%f\n", eval(res.output));
			mpc_ast_delete(res.output);
		} else {
			mpc_err_print(res.error);
			mpc_err_delete(res.error);
		}

	done:
		add_history(line);
		free(line);
	}

	mpc_cleanup(4, num, op, expr, prog);

	return 0;
}
