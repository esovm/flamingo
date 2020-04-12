#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <float.h>

#include "util.h"

// void fuk(const char *fmt, ...)
// {
//     va_list ap;

//     va_start(ap, fmt);
//     vfprintf(stderr, fmt, ap);
//     perror(" ");

//     va_end(ap);
//     exit(1);
// }

char *dupstr(const char *s)
{
    size_t len = strlen(s) + 1;
    char *p = malloc(len);
    return p ? memcpy(p, s, len) : NULL;
}

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
