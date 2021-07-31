#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"

int
Dfmt(Fmt *f)
{
	Device *d;

	d = va_arg(f->args, Device*);
	if(d == nil)
		return fmtprint(f, "<null device>");
	return fmtprint(f, "usb%d/%d", d->ctlrno, d->id);
}

void
usbfmtinit(void)
{
	fmtinstall('D', Dfmt);
}
