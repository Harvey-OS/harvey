#include <u.h>
#include <libc.h>

int
tokenize(char *str, char **args, int max)
{
	int na;

	na = 0;
	while (na < max) {
		while(*str == ' ' && *str != '\0')
			str++;
		args[na++] = str;
		while(!(*str == ' ') && *str != '\0')
			str++;

		if(*str == '\n')
			*str = '\0';

		if(*str == '\0')
			break;

		*str++ = '\0';
	}
	return na;
}
