#include <alef.h>
#include <regexp.h>

void
regerror(byte *s)
{
	byte buf[132];

	strcpy(buf, "regerror: ");
	strcat(buf, s);
	strcat(buf, "\n");
	write(2, buf, strlen(buf));
	exits("regerr");
}
