#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <float.h>

#include "util.h"

void fuk(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    perror(" ");

    va_end(ap);
    exit(1);
}

char *dupstr(const char *s)
{
    size_t len = strlen(s) + 1;
    char *p = malloc(len);

    return p ? memcpy(p, s, len) : NULL;
}

enum str2dbl_errno str2dbl(double *out, char *s)
{
	if (*s == '\0' || isspace((unsigned char)*s))
		return S2D_INCONVERTIBLE;

	errno = 0;
	char *end;
	double d = strtod(s, &end);

	/* FIXME */
	/* if (d > DBL_MAX|| (errno == ERANGE && d == LDBL_MAX))
		return S2D_OVERFLOW;
	if (d < DBL_MIN || (errno == ERANGE && d == LDBL_MIN))
		return S2D_UNDERFLOW; */

	if (*end != '\0')
		return S2D_INCONVERTIBLE;

	*out = d;

	return S2D_SUCCESS;
}

char *trim(char *str)
{
	/* trim leading space */
	while (isspace((unsigned char)*str))
		++str;

	if (*str == '\0') /* all spaces? */
		return str;

	/* trim trailing space */
	char *last = str + strlen(str) - 1;
	while (last > str && isspace((unsigned char)*last))
		--last;

	/* write new null terminator character */
	last[1] = '\0';

	return str;
}

char *readfile(const char *path)
{
    FILE *fp = fopen(path, "r");

	if (!fp) fuk("Cannot open file '%s'", path);

	fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
	rewind(fp);

	char *buf = malloc(len + 1);

	if (!buf) fuk("Couldn't allocate %lu bytes of memory", len + 1);

    buf[len] = '\0';

    fread(buf, 1, len, fp);

	fclose(fp);

    return buf;
}

/*char *concat(const char *s1, ...)
{
    va_list ap;
    size_t total_len = 0;
    const char *str = s1, *result;

    va_start(ap, s1);

    do {
		total_len += strlen(str);
	} while ((str = va_arg(ap, char *)));

    va_end(ap);

    if ((result = malloc(total_len + 1))) {
        va_start(ap, s1);
        str = s1;

        do {
            strcat(result, str);
		} while (str = va_arg(ap, char *));

        va_end(ap);
    }

    return result;
}*/
