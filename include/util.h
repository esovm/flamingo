#ifndef FLAMINGO_UTIL_H
#define FLAMINGO_UTIL_H

#include <stddef.h>
#include <string.h>

#define U_MIN(A, B) (((A) < (B)) ? (A) : (B))
#define U_MAX(A, B) (((A) > (B)) ? (A) : (B))
#define EQ(S1, S2) (strcmp(S1, S2) == 0)

typedef enum {
	S2D_SUCCESS,
	S2D_OVERFLOW,
	S2D_UNDERFLOW,
	S2D_INCONVERTIBLE
} str2dbl_errno;

void fuk(const char *, ...);
void *s_malloc(size_t);
void *s_realloc(void *, size_t);
char *dupstr(const char *);
str2dbl_errno str2dbl(double *, char *);
char *trim(char *);
char *xreadfile(const char *); /* deadly, will quit on failure */
char *readfile(const char *);

#endif /* FLAMINGO_UTIL_H */
