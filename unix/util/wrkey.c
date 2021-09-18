#include <u.h>
#include <libc.h>
#include <auth.h>
#include <authsrv.h>
#include <libsec.h>

void
main(void)
{
	Nvrsafe safe;

	if(readnvram(&safe, NVwrite) < 0)
		sysfatal("error writing nvram: %r");
	exits(0);
}
