#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>

int handled = 0;
int back = 0;
int inhandler = 0;
char *msg = "Writing from NxM program to stdout via linux write(2)\n";

void
handler(void *v, char *s)
{
	inhandler = 1;
	fprint(2, "handler: %p %s\n", v, s);
	handled++;
	inhandler = 0;
	noted(NCONT);
}
main(int argc, char *argv[])
{
	uvlong callbadsys(uvlong, ...);
  
	if (notify(handler)){
		fprint(2, "%r\n");
		exits("notify fails");
	}
	fprint(2, "Notify passed\n");

	callbadsys(0xc0FF);
	fprint(2, "Handled %d\n", handled);

	/* try a good linux system call */
	callbadsys((uvlong)1, (uvlong)1, msg, strlen(msg));
	/* now try a bad linux system call */
	callbadsys(0x3FFF);
	fprint(2, "Handled %d\n", handled);
	back++;

	if (!handled)
		exits("badsyscall test fails");
	return 0;
}
