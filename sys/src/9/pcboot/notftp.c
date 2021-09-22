/* tftp client stub */
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"

#include	"../port/netif.h"
#include	"etherif.h"
#include	"../ip/ip.h"
#include	"pxe.h"
#include	"tftp.h"

void
tftprandinit(void)
{
}

int
tftpboot(Openeth *, char *, Bootp *, Boot *)
{
	return -1;
}

int
tftpopen(Openeth *, char *, Bootp *)
{
	return -1;
}

long
tftprdfile(Openeth *, int, void*, long)
{
	return -1;
}

int
tftpread(Openeth *, Pxenetaddr *, Tftp *, int)
{
	return -1;
}
