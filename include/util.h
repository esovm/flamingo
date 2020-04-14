#ifndef FLAMINGO_UTIL_H
#define FLAMINGO_UTIL_H

#define MIN(A, B) (((A) < (B)) ? (A) : (B))
#define MAX(A, B) (((A) > (B)) ? (A) : (B))

void kill(const char *, ...);
char *dupstr(const char *);
char *trim(char *);
char *readfile(const char *);

#endif /* FLAMINGO_UTIL_H */
