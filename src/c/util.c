#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "util.h"

/* print error to stderr & exit the program */
void kill(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    perror(" ");

    va_end(ap);
    exit(1);
}

/* duplicate string s, caller is responsible for freeing `p` */
char *dupstr(const char *str)
{
    size_t len = strlen(str) + 1;
    char *p = malloc(len);
    return p ? memcpy(p, str, len) : NULL;
}

/* strip both leading & trailing whitespace from string */
char *trim(char *str)
{
	/* trim leading space */
	while (isspace((unsigned char)*str)) ++str;
    /* all spaces? */
	if (*str == '\0') return str;

	/* trim trailing space */
	char *last = &str[strlen(str) - 1];
	while (last > str && isspace((unsigned char)*last)) --last;

	/* write new null terminator character */
	last[1] = '\0';
	return str;
}

/* read contents of `path` into `buf`. return it, or NULL. it's caller responsibility to free `buf` */
char *readfile(const char *path)
{
    FILE *fp = fopen(path, "r");
	if (!fp) return NULL;

	fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    if (len == -1) return NULL;
	rewind(fp);

	char *buf = malloc(len + 1);
	if (!buf) return NULL;

    buf[len] = '\0';
    if (fread(buf, 1, len, fp) != (size_t)len) return NULL;

    fclose(fp);
    return buf;
}

bool is_valid_number_part(char c, int base)
{
    c = tolower(c);
    if (base == 2) return c == '0' || c == '1';
    if (base == 10) return c >= '0' && c <= '9';
    if (base == 16) return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
    return false; /* invalid base */
}
