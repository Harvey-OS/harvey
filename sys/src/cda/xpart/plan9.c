#include <u.h>
#include <libc.h>

int yydebug; /* ignored by plan9 yacc */

char *library = "/sys/lib/cda/library.paddle";

void exit(int code)
{
	exits(code ? "abnormal exit" : (char *) 0);
}
