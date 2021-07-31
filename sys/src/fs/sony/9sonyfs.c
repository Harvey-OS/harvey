#include "all.h"
#include "mem.h"
#include "io.h"
#include "ureg.h"

#include "../pc/dosfs.h"

#ifndef	DATE
#define	DATE	568011600L+4*3600
#endif

ulong	mktime		= DATE;				/* set by mkfile */
Startsb	startsb[] =
{
	"main",		2,
	0
};

Dos dos;

static
struct
{
	char	*name;
	long	(*read)(int, void*, long);
	long	(*seek)(int, long);
	long	(*write)(int, void*, long);
	int	(*part)(int, char*);
} nvrdevs[] =
{
	{ "fd", floppyread, floppyseek, floppywrite, 0, },
	{ "hd", ataread,   ataseek,   atawrite,   setatapart, },
	/*
	{ "sd", scsiread,   scsiseek,   scsiwrite,   setscsipart, },
	 */
	{ 0, },
};

void
otherinit(void)
{
	int dev, i, nfd, nhd, s;
	char *p, *q, buf[sizeof(nvrfile)+8];

	kbdinit();
	printcpufreq();
	etherinit();
	scsiinit();

	s = spllo();
	nhd = atainit();
	nfd = floppyinit();
	dev = 0;
	if(p = getconf("nvr")){
		strncpy(buf, p, sizeof(buf)-2);
		buf[sizeof(buf)-1] = 0;
		p = strchr(buf, '!');
		q = strrchr(buf, '!');
		if(p == 0 || q == 0 || strchr(p+1, '!') != q)
			panic("malformed nvrfile: %s\n", buf);
		*p++ = 0;
		*q++ = 0;
		dev = strtoul(p, 0, 0);
		strcpy(nvrfile, q);
		p = buf;
	} else
	if(p = getconf("bootfile")){
		strncpy(buf, p, sizeof(buf)-2);
		buf[sizeof(buf)-1] = 0;
		p = strchr(buf, '!');
		q = strrchr(buf, '!');
		if(p == 0 || q == 0 || strchr(p+1, '!') != q)
			panic("malformed bootfile: %s\n", buf);
		*p++ = 0;
		*q = 0;
		dev = strtoul(p, 0, 0);
		p = buf;
	} else
	if(nfd)
		p = "fd";
	else
	if(nhd)
		p = "hd";
	else
		p = "sd";

	for(i = 0; nvrdevs[i].name; i++){
		if(strcmp(p, nvrdevs[i].name) == 0){
			dos.dev = dev;
			if(nvrdevs[i].part && (*nvrdevs[i].part)(dos.dev, "disk") == 0)
				break;
			dos.read = nvrdevs[i].read;
			dos.seek = nvrdevs[i].seek;
			dos.write = nvrdevs[i].write;
			break;
		}
	}
	if(dos.read == 0)
		panic("no device for nvram\n");
	if(dosinit(&dos) < 0)
		panic("can't init dos dosfs on %s\n", p);
	splx(s);
}

void
touser(void)
{
	int i;

	settime(rtctime());
	boottime = time();

	print("sysinit\n");
	sysinit();

	userinit(floppyproc, 0, "floppyproc");
	/*
	 * Ethernet i/o processes
	 */
	etherstart();


	/*
	 * read ahead processes
	 */
	userinit(rahead, 0, "rah");

	/*
	 * server processes
	 */
	for(i=0; i<conf.nserve; i++)
		userinit(serve, 0, "srv");

	/*
	 * worm "dump" copy process
	 */
	userinit(wormcopy, 0, "wcp");

	/*
	 * processes to read the console
	 */
	consserve();

	/*
	 * "sync" copy process
	 * this doesn't return.
	 */
	u->text = "scp";
	synccopy();
}

void
localconfinit(void)
{
	conf.nfile = 40000;
	conf.wcpsize = 10;
	conf.nodump = 0;
	conf.firstsb = 13219302;
	conf.recovsb = 0;
	conf.ripoff = 1;
	conf.nlgmsg = 100;
	conf.nsmmsg = 500;
}

int (*fsprotocol[])(Msgbuf*) = {
	serve9p1,
	serve9p2,
	nil,
};
