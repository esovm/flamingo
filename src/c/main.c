#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>

#include "util.h"
#include "object.h"
#include "env.h"

#define FLAMINGO_VERSION "0.3.0"

int main(int argc, char **argv)
{
    Env *env = env_new();
    env_register_all(env);

    if (argc == 1) {
        printf("Flamingo %s\ntype \"exit <status>\" to terminate\n", FLAMINGO_VERSION);
        char *line;
        while ((line = readline("=> "))) {
            /* ignore comments and empty/whitespace-only strings */
            if (!*line || !*trim(line) || *trim(line) == '#') {
                free(line);
                continue;
            }
            int pos = 0;
            Object *obj = obj_eval(env, obj_read_expr(line, &pos, '\0'));
            OBJ_DUMP_LN(obj);
            obj_free(obj);
            add_history(line);
            free(line);
        }
        putchar('\n');
    } else {
        for (int i = 1; i < argc; ++i) {
            Object *r, *args = obj_append(obj_new_sexpr(), obj_new_str(argv[i]));
            if ((r = bi_use(env, args))->type == O_ERROR)
                OBJ_DUMP_LN(r);
            obj_free(r);
        }
    }
    env_free(env);
    return 0;
}
