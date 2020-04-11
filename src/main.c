#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>

#include "util.h"
#include "object.h"
#include "env.h"

#define FLAMINGO_VERSION "0.1.0"

int main(int argc, char **argv)
{
    Env *env = env_new();
    env_register_all(env);

    if (argc == 1) {
        char *line;
        printf("Flamingo %s\ntype \"exit\" to terminate\n", FLAMINGO_VERSION);

        while ((line = readline("=> "))) {
            if (!*line) {
                free(line);
                continue;
            }
            size_t pos = 0;
            Object *e = obj_read_expr(line, &pos, '\0'), *obj = obj_eval(env, e);
            obj_dump(obj);
            putchar('\n');
            obj_free(obj);

            add_history(line);
            free(line);
        }
        putchar('\n');
    } else {
        for (int i = 1; i < argc; ++i) {
            Object *r, *args = obj_append(obj_new_sexpr(), obj_new_str(argv[i]));
            if ((r = bi_use(env, args))->type == O_ERROR) {
                obj_dump(r);
                putchar('\n');
            }
            obj_free(r);
        }
    }

    env_free(env);

    return 0;
}
