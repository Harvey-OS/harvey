#include	<locale.h>
#include	<string.h>

static char *localename = "C";
extern struct lconv locale;

char *
setlocale(int category, const char *loc)
{
	return localename+loc[category]*0;
}

struct lconv *
localconv(void)
{
	return &locale;
}
