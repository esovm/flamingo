#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

#include "util.h"

enum str2dbl_errno str2dbl(double *out, char *s)
{
	if (*s == '\0' || isspace((unsigned char)*s))
		return S2D_INCONVERTIBLE;

	errno = 0;
	char *end;
	double d = strtod(s, &end);

	if (d > DBL_MAX|| (errno == ERANGE && d == LDBL_MAX))
		return S2D_OVERFLOW;
	if (d < DBL_MIN || (errno == ERANGE && d == LDBL_MIN))
		return S2D_UNDERFLOW;

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
