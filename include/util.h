#ifndef FLAMINGO_UTIL_H
#define FLAMINGO_UTIL_H

#include <stdbool.h>

void kill(const char *, ...);
char *dupstr(const char *);
char *trim(char *);
char *readfile(const char *);
bool is_valid_number_part(char, int);

#endif /* FLAMINGO_UTIL_H */
