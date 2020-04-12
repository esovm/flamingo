#ifndef FLAMINGO_UTIL_H
#define FLAMINGO_UTIL_H

#include <stddef.h>
#include <string.h>

#define U_MIN(A, B) (((A) < (B)) ? (A) : (B))
#define U_MAX(A, B) (((A) > (B)) ? (A) : (B))
#define EQ(S1, S2) (strcmp(S1, S2) == 0)

// void fuk(const char *, ...);
char *dupstr(const char *);
char *trim(char *);
char *readfile(const char *);

#endif /* FLAMINGO_UTIL_H */
