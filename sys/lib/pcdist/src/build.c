#include <u.h>
#include <libc.h>
#include "dat.h"

enum
{
	CDSIZE		= 500*1024*1024,
	FLSIZE		= 14*1024*1024,
};

typedef struct Buffer Buffer;
struct Buffer
{
	int	size;
	int	mode;
	char*	data;
	char*	path;
};

uchar*	cga;
char	kbdc[10];
int	kbdcnt;
int	cfd;
int	kfd;
char*	window[NLINES+1];
int	docmd(char *av[], char*);
void	newwindow(Saved*);
int	ipstarted;

int	chkinstall(void);
void	install(void);
void	scroll(char*);
void	rcp(char*, char*, Dir*);
int	copy(char*, char*, char*, char*, Dir*);

char	anykey[] = "Press any key to continue ...";

void
xclrscrn(int attr)
{
	int i;

	for(i = 0; i < CGAWIDTH*CGAHEIGHT; i += 2) {
		cga[i] = ' ';
		cga[i+1] = attr;
	}
}

void
xprint(int x, int y, int attr, char *fmt, ...)
{
	uchar *p;
	int out, i;
	char buf[CGAWIDTH];

	x *= 2;

	out = doprint(buf, buf+sizeof(buf), fmt, DOTDOT) - buf;
	p = (uchar*)(cga + x + (y*CGAWIDTH));
	for(i = 0; i < out; i++) {
		*p++ = buf[i];
		*p++ = attr;
	}
}

void
xputc(int x, int y, int attr, int c)
{
	uchar *p;

	x *= 2;
	p = (uchar*)(cga + x + (y*CGAWIDTH));
	p[0] = c;
	p[1] = attr;
}

void
restore(Saved *s)
{
	int l;
	char *sv, *cm;

	sv = s->store;
	for(l = 0; l < s->nitems; l++) {
		cm = (char*)cga + (s->x*2) + ((s->y+l)*CGAWIDTH);
		memmove(cm, sv, s->wid*2);
		sv += s->wid*2;
	}
	free(s->store);
}

void
infobox(int x, int y, char *items[], Saved *s)
{
	int nitem;
	int i, wid, l;
	char *store, *sv, *cm;

	wid = 0;
	for(i = 0; items[i] != 0; i++) {
		l = strlen(items[i]);
		if(l > wid)
			wid = l;
	}
	nitem = i;

	sv = 0;
	store = 0;
	if(s != 0) {
		store = malloc((wid+2)*(i+2)*2);
		if(store == 0) {
			fprint(2, "install: no memory");
			exits("malloc");
		}
		sv = store;
	}

	for(l = -1; l < nitem+1; l++) {
		cm = (char*)cga + ((x-1)*2) + ((y+l)*CGAWIDTH);
		if(sv != 0) {
			memmove(sv, cm, (wid+2)*2);
			sv += (wid+2)*2;
		}
		cm += 2;
		for(i = 0; i < wid; i++) {
			*cm++ = ' ';
			*cm++ = MENUATTR;
		}		
	}
	if(s != 0) {
		s->store = store;
		s->nitems = nitem+2;
		s->x = x-1;
		s->y = y-1;
		s->wid = wid+2;
	}

	for(i = 0; i < wid; i++) {
		xputc(x+i, y-1, MENUATTR, 0xc4);
		xputc(x+i, y+nitem, MENUATTR, 0xc4);
	}
	for(i = 0; items[i] != 0; i++) {
		xputc(x-1, y+i, MENUATTR, 0xb3);
		xputc(x+wid, y+i, MENUATTR, 0xb3);
		xprint(x, y+i, MENUATTR, "%s", items[i]);
	}
	xputc(x-1, y-1, MENUATTR, 0xda);
	xputc(x+wid, y-1, MENUATTR, 0xbf);
	xputc(x-1, y+nitem, MENUATTR, 0xc0);
	xputc(x+wid, y+nitem, MENUATTR, 0xd9);
}

int
getc(void)
{
	Rune r;
	int n, w;

	while(!fullrune((char*)kbdc, kbdcnt)) {
		n = read(kfd, kbdc+kbdcnt, 1);
		if(n < 1) {
			fprint(2, "install: eof or error on input: %r\n");
			exits("read");
		}
		kbdcnt += n;
	}
	w = chartorune(&r, (char*)kbdc);
	memmove(kbdc, kbdc+w, kbdcnt-w);
	kbdcnt -= w;

	return r;
}

int
menu(int x, int y, Menu *menu, Saved *s)
{
	char *cm;
	int r, nitem, choice, i, wid;

	infobox(x, y, menu->items, s);

	nitem = s->nitems-2;
	wid = s->wid-2;
	choice = menu->choice;
	for(;;) {
		cm = (char*)cga + (x*2) + ((y+choice)*CGAWIDTH) + 1;
		for(i = 0; i < wid; i++) {
			*cm = HIGHMENUATTR;
			cm += 2;
		}
		r = getc();

		if(r == 'q') {
			for(i = 0; i < CGAWIDTH*CGAHEIGHT; i += 2) {
				cga[i] = ' ';
				cga[i+1] = 0;
			}
			exits(0);
		}

		if(r == '\n') {
			menu->selected = 1;
			break;
		}
		if(r == 0x1b)
			return menu->choice;

		cm = (char*)cga + (x*2) + ((y+choice)*CGAWIDTH) + 1;
		for(i = 0; i < wid; i++) {
			*cm = MENUATTR;
			cm += 2;
		}

		switch(r) {
		case 128:
		case '\t':			/* Arrow down */
			if(choice < nitem-1)
				choice++;
			else
				choice = 0;	
			break;
		case 206:			/* Arrow up */
			choice--;
			if(choice < 0)
				choice = nitem-1;
			break;
		}
	}

	menu->choice = choice;
	return choice;	
}

char *cdi[] =
{
	"Choose a hard drive to install the system on.",
	"The following types are supported:",
	"      hd0-1 are IDE drives",
	"      sd0-7 are SCSI drives",
	0,
};

char *nodisks[] =
{
	"The system found no disks suitable to perform",
	"the installation.  If you expected to use a",
	"SCSI disc, check that you have configured the",
	"device correctly.",
	anykey,
	0,
};

char *already[] =
{
	"The drive has a Plan 9 file system installed",
	"already.",
	0,
};

Menu proceed =
{
	0, 0,

	"Abort",
	"Overwrite existing system",
};

int
disksel(char *dev, int chk)
{
	Menu ld;
	int c, l, i;
	Saved s1, s2;
	char buf[64], drives[NMENU][32];

	dev[0] = '\0';

	l = 0;
	memset(&ld, 0, sizeof(Menu));
	for(i = 0; i < 4; i++) {
		sprint(drives[l], "/dev/hd%ddisk", i);
		if(access(drives[l], 0) == 0) {
			sprint(drives[l], "/dev/hd%d", i);
			ld.items[l++] = drives[l];
		}
	}
	for(i = 0; i < 8; i++) {
		sprint(drives[l], "/dev/sd%ddisk", i);
		if(access(drives[l], 0) == 0) {
			sprint(drives[l], "/dev/sd%d", i);
			ld.items[l++] = drives[l];
		}
	}

	if(l == 0) {
		infobox(7, 7, nodisks, &s1);
		getc();
		restore(&s1);
		return 0;
	}

	infobox(7, 7, cdi, &s1);
	c = menu(9, 12, &ld, &s2);
	restore(&s2);
	restore(&s1);
	if(ld.selected == 0)
		return 0;

	if(chk) {
		sprint(buf, "%sfs", drives[c]);
		if(access(buf, 0) == 0) {
			infobox(10, 10, already, &s1);
			i = menu(13, 13, &proceed, &s2);
			restore(&s2);
			restore(&s1);
			if(i == 0)
				return 0;
		}
	}
	strcpy(dev, drives[c]);
	return 1;
}

char *prep[] =
{
	"Invoking disk/prep to build Plan 9",
	"partition tables for device:",
	0,
	0
};

char *preperr[] =
{
	"An error  occurred while trying  to build",
	"a Plan 9 partition table for the disc. The",
	"most  likely cause  is insufficient  disc",
	"space.",
	anykey,
	0
};

int
prepdisk(char *disk)
{
	Saved s1, s2;
	char *av[10];

	if(disksel(disk, 1) == 0)
		return -1;

	prep[2] = disk;
	infobox(7, 7, prep, &s1);

	av[0] = "/bin/disk/prep";
	av[1] = "-a";
	av[2] = disk;
	av[3] = 0;
	if(docmd(av, 0) == -1) {
		infobox(10, 10, preperr, &s2);
		getc();
		restore(&s2);
		restore(&s1);
		return -1;
	}
	restore(&s1);
	return 0;
}

char *se[] =
{
	"The system encountered the following error:",
	0,
	"while trying execute the command:",
	0,
	anykey,
	0
};

char *skfs[] =
{
	"Starting up local file system.",
	"Please wait while a new file system is created ...",
	0
};

char *s9660[] =
{
	"Starting up 9660 file system for CD-ROM.",
	0
};

int
startkfs(char *disk)
{
	int fd;
	Saved s1, s2;
	char *av[10], buf[64];

	infobox(10, 10, skfs, &s1);

	sprint(buf, "%sfs", disk);

	if(access("#s/kfs", ORDWR) == -1) {
		av[0] = "/bin/disk/kfs";
		av[1] = "-rb4096";
		av[2] = "-f";
		av[3] = buf;
		av[4] = 0;
		if(docmd(av, 0) == -1) {
			restore(&s1);
			return -1;
		}
		av[0] = "/bin/disk/kfscmd";
		av[1] = "allow";
		av[2] = 0;
		if(docmd(av, 0) == -1) {
			restore(&s1);
			return -1;
		}
	}
	fd = open("#s/kfs", ORDWR);
	if(fd < 0) {
		errstr(buf);
		se[1] = buf;
		se[3] = "Opening the kfs service file";
		infobox(12, 12, se, &s2);
		getc();
		restore(&s2);
		restore(&s1);
		return -1;
	}
	if(mount(fd, "/n/kfs", MREPL|MCREATE, "") < 0) {
		errstr(buf);
		se[1] = buf;
		se[3] = "Mounting the kfs file system";
		infobox(12, 12, se, &s2);
		getc();
		restore(&s2);
		restore(&s1);
		return -1;
	}
	restore(&s1);
	return 0;
}

int
start9660(char *disk)
{
	int fd;
	Saved s1, s2;
	char *av[10], buf[64];

	infobox(10, 10, s9660, &s1);

	if(access("#s/9660", ORDWR) == -1) {
		av[0] = "/bin/9660srv";
		av[1] = 0;
		if(docmd(av, 0) == -1) {
			restore(&s1);
			return -1;
		}
	}
	fd = open("#s/9660", ORDWR);
	if(fd < 0) {
		errstr(buf);
		se[1] = buf;
		se[3] = "Opening the 9660 service file";
		infobox(12, 12, se, &s2);
		getc();
		restore(&s2);
		restore(&s1);
		return -1;
	}
	if(mount(fd, "/n/cdrom", MREPL, disk) < 0) {
		buf[0] = '\0';
		errstr(buf);
		se[1] = buf;
		se[3] = "Mounting the 9660 file system";
		infobox(12, 12, se, &s2);
		getc();
		restore(&s2);
		restore(&s1);
		return -1;
	}
	restore(&s1);
	return 0;
}

char *nokfs[] =
{
	"Installation cannot proceed without",
	"the kfs file server.",
	anykey,
	0,
};

char *no9660[] =
{
	"Installation cannot proceed without",
	"the 9660 file server.",
	anykey,
	0,
};

char *edosfs[] =
{
	"An error occurred connecting to the DOS",
	"file system.  The error was:",
	0,
	"Installation cannot proceed.",
	anykey,
	0
};

char *emount[] =
{
	"An error occurred mounting the DOS disc",
	"in drive A:.  The error was:",
	0,
	"Check you have the correct disk inserted.",
	0
};

Menu retry =
{
	0, 0,

	"Retry",
	"Abort",
};

char *edisk[] =
{
	"You have placed the wrong diskette in",
	"drive A:.  Please insert the correct",
	"diskette.",
	anykey,
	0
};

char *eexpand[] =
{
	"An error occurred while decompressing",
	"and installing the disc.",
	0
};

int
dodisk(int disk)
{
	int c, fd;
	Saved s1, s2, s3;
	char buf[32], err[ERRLEN], *fn[4], *av[10];

	sprint(buf, "Place diskette %d in drive A:", disk);
	fn[0] = buf;
	fn[1] = anykey;
	fn[2] = 0;

	infobox(9, 9, fn, &s1);
	getc();
retry:
	unmount(0, "/n/a");
	fd = open("/srv/dos", ORDWR);
	if(fd < 0) {
		err[0] = '\0';
		errstr(err);
		edosfs[2] = err;
		infobox(12, 12, edosfs, &s2);
		getc();
		restore(&s2);
		restore(&s1);
		return -1;
	}
	if(mount(fd, "/n/a", MREPL, "/dev/fd0disk") == -1) {
		err[0] = '\0';
		errstr(err);
		emount[2] = err;
		infobox(12, 12, emount, &s2);
		retry.choice = 0;
		c = menu(22, 18, &retry, &s3);
		restore(&s3);
		restore(&s2);
		if(c == 1) {
			restore(&s1);
			return -1;
		}
		goto retry;
	}

	sprint(buf, "/n/a/disk%d.vd", disk);
	if(access(buf, 0) != 0) {
		infobox(12, 12, edisk, &s2);
		getc();
		restore(&s2);
		unmount(0, "/n/a");
		goto retry;
	}

	sprint(buf, "vdexpand < /n/a/disk%d.vd | disk/mkext -uv -d/n/kfs", disk);
	av[0] = "/bin/rc";
	av[1] = "-c";
	av[2] = buf;
	av[3] = 0;
	if(docmd(av, "Expanding archive") == -1) {
		retry.choice = 0;
		infobox(12, 12, eexpand, &s2);
		c = menu(15, 15, &retry, &s3);
		restore(&s3);
		restore(&s2);
		if(c == 1) {
			unmount(0, "/n/a");
			restore(&s1);
			return -1;
		}
		goto retry;
	}

	restore(&s1);
	return 0;
}

char *nospace[] =
{
	"There is insufficient disc space after",
	"the DOS partitions to install the system.",
	0,
	anykey,
	0,
};

char *ikernel[] =
{
	"Installing a kernel in the boot",
	"partition.",
	"Please wait ...",
	0
};

void
copykernel(char *xdisk)
{
	Saved s1;
	char *p, buf[NAMELEN], disk[ERRLEN];

	infobox(WLOCX+5, WLOCY+5, ikernel, &s1);

	strcpy(disk, xdisk);
	p = strrchr(disk, '/');
	if(p != 0) {
		*p = '\0';
		sprint(buf, "%sboot", p+1);
	}
	else {
		werrstr("bad device file");
		warn(disk);
		restore(&s1);
		return;
	}

	if(copy("/n/kfs/386", "9pcdisk", disk, buf, 0) != 0)
		warn("/n/kfs/386/9pcdisk");
	restore(&s1);
}

void
install4d(void)
{
	Dir d;
	Saved s1;
	char disk[64], buf[129];

	if(prepdisk(disk) != 0)
		return;

	sprint(buf, "%sfs", disk);
	if(dirstat(buf, &d) < 0) {
		infobox(10, 10, preperr, &s1);
		getc();
		restore(&s1);
		return;
	}
	if(d.length < FLSIZE) {
		sprint(buf, "You need to free %d bytes.", FLSIZE - d.length);
		nospace[2] = buf;
		infobox(10, 10, nospace, &s1);
		getc();
		restore(&s1);
		return;
	}

	if(startkfs(disk) != 0) {
		infobox(10, 10, nokfs, &s1);
		getc();
		restore(&s1);
		return;
	}

	if(dodisk(2) != 0)
		return;
	if(dodisk(3) != 0)
		return;
	if(dodisk(4) != 0)
		return;

	copykernel(disk);

	itsdone(disk);
}

Menu instdone =
{
	0, 0,

	"Make the newly installed Plan 9 the default",
	"Reconfigure the system",
	"Exit to system",
	"Reboot"
};

void
itsdone(char *disk)
{
	Saved s1;

	instdone.selected = 0;
	for(;;) {
		switch(menu(10, 10, &instdone, &s1)) {
		case 0:
			fixplan9("/n/c/plan9/plan9.ini", disk);
			instdone.choice = 3;
			break;		
		case 1:
			configure("/n/c/plan9/plan9.ini", 0, 0);
			instdone.choice = 3;
			break;
		case 2:
			xclrscrn(15);
			exits(0);	
		case 3:
			reboot(0);
		}
		restore(&s1);
	}
}

Menu cdtype = 
{
	0, 0,

	"SCSI CD-ROM",
	"Soundblaster Panasonic CD-ROM",
	"Soundblaster Matsushita CD-ROM",
	"Soundblaster Mitsumi CD-ROM",

};

char *putincd[] =
{
	"Please place the distribution CD-ROM",
	"in the drive.",
	anykey,
	0
};

char *nocd[] =
{
	"The CD-ROM device could not be found.",
	0,
	"The error message was:",
	0,
	"Check you have configured the device in",
	"the System Installation & Configuration,",
	"and that the CD is already in the drive",
	"You can reconfigure by rebooting with",
	"diskette 1 and selecting Reconfigure",
	"from the first menu.", 
	anykey,
	0,
};

Menu unit = 
{
	8, 0,

	"unit 0",
	"unit 1",
	"unit 2",
	"unit 3",
	"unit 4",
	"unit 5",
	"unit 6",
	"unit 7",
	"probe",
};

char *probing[] =
{
	"Probing SCSI devices."
	"Please wait ...",
	0,
};

int
scsiunit(void)
{
	Saved s1;
	int i, c;
	char *iv[9];

	for(;;) {
		unit.selected = 0;
		c = menu(23, 12, &unit, &s1);
		if(unit.selected == 0) {
			restore(&s1);
			return -1;
		}
		restore(&s1);
		if(c <= 7)
			return c;

		infobox(23, 12, probing, &s1);
		for(i = 0; i < 8; i++)
			iv[i] = malloc(NWIDTH);
		probe(iv);
		restore(&s1);

		infobox(12, 12, iv, &s1);
		getc();
		restore(&s1);
		for(i = 0; i < 8; i++) {
			if(strstr(iv[i], "CD-ROM") != 0)
				unit.choice = i;
			free(iv[i]);
		}
	}
	return -1;
}

int
cdromsel(char *cdrom)
{
	Saved s1;
	int i, fd, c, unit;
	char err[ERRLEN], e1[3*NAMELEN], buf[3*NAMELEN];

	infobox(8, 8, putincd, &s1);
	getc();
	restore(&s1);

	cdtype.selected = 0;
	c = menu(10, 10, &cdtype, &s1);
	if(cdtype.selected == 0) {
		restore(&s1);
		return -1;
	}

	if(c == 0) {
		unit = scsiunit();
		restore(&s1);
		if(unit == -1)
			return -1;

		sprint(buf, "#R%d/cd%d", unit, unit);
	}
	else {
		restore(&s1);
		strcpy(buf, "#m/cd");
	}
	/* The sb16 CD-ROM device drivers clear open door and
	 * disc change in opens
	 */
	fd = -1;
	for(i = 0; i < 3; i++) {
		fd = open(buf, OREAD);
		if(fd >= 0)
			break;
	}
	if(fd < 0) {
		sprint(e1, "The device file was %s", buf);
		nocd[1] = e1;
		err[0] = '\0';
		errstr(err);
		nocd[3] = err;
		infobox(10, 10, nocd, &s1);
		getc();
		restore(&s1);
		return -1;
	}	
	close(fd);
	strcpy(cdrom, buf);
	return 0;
}

void
installcd(void)
{
	Dir d;
	Saved s1;
	char disk[64], cdrom[64], buf[3*NAMELEN];

	if(prepdisk(disk) != 0)
		return;

	sprint(buf, "%sfs", disk);
	if(dirstat(buf, &d) < 0) {
		infobox(10, 10, preperr, &s1);
		getc();
		restore(&s1);
		return;
	}
	if(d.length < CDSIZE) {
		sprint(buf, "You need to free %d bytes.", CDSIZE - d.length);
		nospace[2] = buf;
		infobox(10, 10, nospace, &s1);
		getc();
		restore(&s1);
		return;
	}

	if(startkfs(disk) != 0) {
		infobox(10, 10, nokfs, &s1);
		getc();
		restore(&s1);
		return;
	}

	if(cdromsel(cdrom) != 0)
		return;

	if(start9660(cdrom) != 0) {
		infobox(10, 10, no9660, &s1);
		getc();
		restore(&s1);
		return;
	}

	newwindow(&s1);
	rcp("/n/cdrom", "/n/kfs", 0);
	restore(&s1);

	copykernel(disk);

	itsdone(disk);
}

#define CLASS(p) ((*(uchar*)(p))>>6)

int
checkip(char *ip, int strong, uchar *to)
{
	int i, n;
	char *p, *op;

	p = (char*)ip;
	memset(to, 0, 4);
	for(i = 0; i < 4 && *p; i++){
		op = p;
		n = strtoul(p, &p, 0);
		if(n > 255 || p == op)
			return -1;
		to[i] = n;
		if(*p != '.' && *p != '\0')
			break;
		p++;
	}
	if(strong == 0)
		return 0;

	switch(CLASS(to)){
	case 0:	/* class A - 1 byte net */
	case 1:
		if(i < 2 || i > 4)
			return -1;
		break;
	case 2:	/* class B - 2 byte net */
		if(i != 3 && i != 4)
			return -1;
		break;
	case 3:
		if(i != 4)
			return -1;
		break;
	}
	return 0;
}

char *invip[] =
{
	"The address you have entered is",
	"not a valid IP address. Try again.",
	anykey,
	0,
};

int
getname(char *msg[], char *val, int len)
{
	int c, i;
	Saved s1;

	infobox(10, 10, msg, &s1);
	xputc(21, 15, 143, 174);
	i = 0;
	while(val[i] != 0) {
		xputc(21+i, 15, 15, val[i++]);
		xputc(21+i, 15, 143, 174);
	}
	for(;;) {
		c = getc();
		switch(c) {
		case 0x1b:
			restore(&s1);
			return 0;
		case '\r':
		case '\n':
			val[i] = '\0';
			restore(&s1);
			return 1;
		case '\b':
			xputc(21+i, 15, 15, ' ');
			i--;
			if(i < 0)
				i = 0;
			xputc(21+i, 15, 143, 174);
			break;
		case 0x15:
			while(i) {
				xputc(21+i, 15, 15, ' ');
				i--;
				xputc(21+i, 15, 143, 174);
			}
			break;
		default:
			if(i+1 >= len)
				break;
			xputc(21+i, 15, 15, c);
			val[i++] = c;
			xputc(21+i, 15, 143, 174);
			break;
		}
	}
	restore(&s1);
	return 0;	
}

int
getipa(char *msg[], char *ip, int strong, uchar *to)
{
	Saved s1;

	for(;;) {
		if(getname(msg, ip, 16) == 0)
			return 0;
		if(checkip(ip, strong, to) == 0)
			return 1;

		infobox(15, 17, invip, &s1);
		getc();
		restore(&s1);
	}
	return 0;
}

char *myip[] =
{
	"Enter the IP address of this system.",
	"If you make a mistake you will have",
	"to reboot in order to change the IP",
	"address.",
	"THIS MACHINE'S",
	"IP Address:",
	"",
	0,
};

char *gwip[] =
{
	"Enter the IP address of the gateway.",
	"If you make a mistake you will have",
	"to reboot in order to change this IP",
	"address.",
	"GATEWAY'S",
	"IP Address:",
	"",
	0,
};

char *mskip[] =
{
	"Enter the IP mask you wish to use for",
	"the IP address you have selected. The",
	"system will suggest a default.",
	"",
	"",
	"IP Mask   :",
	"",
	0,
};

char *fsip[] =
{
	"Enter the IP address of the file server",
	"you want to install the CD-ROM on.",
	"",
	"",
	"REMOTE SERVER'S",
	"IP Address:",
	"",
	0,
};

char *usefs[] =
{
	"Enter the IP address of the file server",
	"you have set up to serve this CPU server.",
	"",
	"",
	"REMOTE SERVER'S",
	"IP Address:",
	"",
	0,
};

char *fsname[] =
{
	"Enter the fully qualified domain name of the file server",
	"Example: machine1.fatcats.com",
	"",
	"",
	"REMOTE SERVER'S HOSTNAME",
	"Hostname  :                                             ",
	"",
	0,
};

char *baddom[] =
{
	"The domain name must include at least one period.",
	anykey,
	0
};

char *classmask[4] = {
	"255.0.0.0",
	"255.0.0.0",
	"255.255.0.0",
	"255.255.255.0",
};

char *ipcheck[] =
{
	"You have selected the following",
	"IP addresses/IP names:",
	"",
	0,
	0,
	0,
	0
};

Menu ipedit =
{
	0, 0,

	"Redo IP address",
	"Proceed"
};

char *noip[] =
{
	"The installation cannot proceed",
	"without IP services.",
	anykey,
};

char *calling[] =
{
	"Dialing the remote file server",
	0,
};

char *baddial[] =
{
	"The system encountered an error dialing",
	"the address:",
	0,
	"The system error message was:",
	0,
	anykey,
	0
};

char *nogw[] =
{
	"There was an error setting the gateway",
	"address. You cannot contact machines on",
	"other networks, but IP will work to machines",
	"on the same net",
	anykey,
};

int
getfile(Buffer *b, char *path)
{
	Dir d;
	int fd;

	b->path = path;
	fd = open(path, ORDWR);
	if(fd < 0) {
		warn(path);
		return -1;
	}
	if(dirfstat(fd, &d) < 0) {
		close(fd);
		warn(path);
		return -1;
	}
	b->size = d.length;
	b->mode = d.mode;
	b->data = malloc(b->size);
	if(b->data == 0) {
		close(fd);
		warn(path);
		return -1;
	}
	if(read(fd, b->data, b->size) < 0) {
		close(fd);
		warn(path);
		return -1;
	}
	close(fd);
	return 0;		
}

int
putfile(Buffer *b)
{
	int fd;

	fd = create(b->path, OWRITE, b->mode);
	if(fd < 0) {
		free(b->data);
		warn(b->path);
		return -1;
	}
	if(write(fd, b->data, b->size) != b->size) {
		free(b->data);
		warn(b->path);	
		close(fd);
		return -1;
	}
	free(b->data);
	close(fd);
	return 0;
}

char *nomem[] =
{
	"Malloc failed",
	anykey,
	0,
};

char *editf[] =
{
	"Failed to edit configuration file:",
	0,
	"The reason was:",
	0,
	anykey,
	0,
};

int
replace(Buffer *b, char *key, char *repl)
{
	Saved s1;
	char *newbuf, *p, *q;

	newbuf = malloc(b->size+strlen(repl));
	if(newbuf == 0) {
		infobox(10, 10, nomem, &s1);
		getc();
		restore(&s1);
		return -1;
	}

	p = strstr(b->data, key);
	if(p == 0) {
		editf[1] = b->path;
		editf[3] = "File does not have #INSTALL key";
		infobox(10, 10, editf, &s1);
		getc();
		restore(&s1);
		free(newbuf);
		return -1;
	}
	p += strlen(key);

	memmove(newbuf, b->data, p - b->data);
	q = newbuf + (p - b->data);
	memmove(q, repl, strlen(repl));
	q += strlen(repl);

	p = strstr(p, "#END");
	if(p == 0) {
		editf[1] = b->path;
		editf[3] = "File does not have #END for #INSTALL";
		infobox(10, 10, editf, &s1);
		getc();
		restore(&s1);
		free(newbuf);
		return -1;
	}
	memmove(q, p, strlen(p)+1);
	free(b->data);
	b->data = newbuf;
	b->size = strlen(newbuf);
	return 0;
}

int
getether(uchar *ip, char *ea)
{
	int fd, n;
	char *p, ipbuf[NAMELEN];
	static char arpdata[8192];

	strcpy(ea, "080000000000");
	fd = open("#a/arp/data", OREAD);
	if(fd < 0)
		return -1;

	n = read(fd, arpdata, sizeof(arpdata));
	close(fd);	
	if(n < 0)
		return -1;

	arpdata[n-1] = '\0';
	sprint(ipbuf, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
	p = strstr(arpdata, ipbuf);
	if(p == 0)
		return -1;

	p = strstr(p, "to ");
	p += 3;
	while(*p && *p != ' ') {
		if(*p != ':')
			*ea++ = *p;
		p++;
	}
	*ea = '\0';
	return 0;
}

char*
samenet[] =
{
	"You can only install a system onto a single network",
	"using this software.  It was designed to get you up",
	"and running.  Look at the manuals to see how to set",
	"up a system that spans multiple IP networks.",
	anykey,
	0,
};

static uchar ipb[4], ipgwb[4], ipmb[4], fsipb[4];
static char ipaddr[NAMELEN], ipgw[NAMELEN], ipmask[NAMELEN];

int
startip(void)
{
	int c, fd;
	Saved s1, s2;
	char i1[128], i2[128], i3[128], *av[10];

	if(ipstarted)
		return 0;

	for(;;) {
		ipaddr[0] = '\0';
		if(getipa(myip, ipaddr, 1, ipb) == 0)
			return -1;
		ipgw[0] = '\0';
		if(getipa(gwip, ipgw, 1, ipgwb) == 0)
			return -1;
		strcpy(ipmask, classmask[CLASS(ipb)]);
		if(getipa(mskip, ipmask, 0, ipmb) == 0)
			return -1;

		sprint(i1, "System      IP address: %s", ipaddr);
		sprint(i2, "Gateway     IP address: %s", ipgw);
		sprint(i3, "Network Mask          : %s", ipmask);
		ipcheck[3] = i1;
		ipcheck[4] = i2;
		ipcheck[5] = i3;
		infobox(10, 10, ipcheck, &s1);
		ipedit.choice = 0;
		c = menu(15, 17, &ipedit, &s2);
		restore(&s2);
		restore(&s1);
		if(c == 1)
			break;
	}
	av[0] = "/bin/ip/ipconfig";
	av[1] = "-b";
	av[2] = "-m";
	av[3] = ipmask;
	av[4] = ipaddr;
	av[5] = 0;
	if(docmd(av, 0) == -1) {
		infobox(10, 10, noip, &s1);
		getc();
		restore(&s1);
		return -1;
	}
	sprint(i1, "add 0.0.0.0 0.0.0.0 %s", ipgw);
	fd = open("#P/iproute", OWRITE);
	if(write(fd, i1, strlen(i1)) < 0) {
		infobox(10, 10, nogw, &s1);
		getc();
		restore(&s1);
	}
	close(fd);
	ipstarted = 1;
	return 0;
}

int
connectfs(char *host, char *domain, int size, char *msg[])
{
	Menu dm;
	int c, fd, cd;
	Saved s1, s2;
	char *p, i1[128], i2[128], i3[128];

	for(;;) {
		ipaddr[0] = '\0';
		if(getipa(msg, ipaddr, 1, fsipb) == 0)
			return -1;

		for(c = 0; c < 4; c++) {
			if((fsipb[c]&ipmb[c]) != (ipgwb[c]&ipmb[c])) {
				infobox(10, 10, samenet, &s1);
				getc();
				restore(&s1);
				return -1;
			}
		}

		domain[0] = '\0';
		if(getname(fsname, domain, size) == 0)
			return -1;

		p = strchr(domain, '.');
		if(p == 0) {
			infobox(10, 10, baddom, &s1);
			getc();
			restore(&s1);
			continue;
		}

		*p++ = '\0';
		strcpy(host, domain);
		strcpy(domain, p);

		sprint(i1, "File System IP address: %s", ipaddr);
		sprint(i2, "File Server Hostname  : %s", host);
		sprint(i3, "File Server Domain    : %s", domain);
		ipcheck[3] = i1;
		ipcheck[4] = i2;
		ipcheck[5] = i3;
		infobox(10, 10, ipcheck, &s1);
		ipedit.choice = 0;
		c = menu(15, 17, &ipedit, &s2);
		restore(&s2);
		restore(&s1);
		if(c == 1)
			break;
	}

	memset(&dm, 0, sizeof(Menu));
	sprint(i1, "il!%s!17008  Plan 9 File server using IL", ipaddr);
	sprint(i2, "tcp!%s!564   u9fs server on UNIX using TCP", ipaddr);
	dm.items[0] = i1;
	dm.items[1] = i2;

	c = menu(10, 10, &dm, &s1);
	restore(&s1);
	if(dm.selected == 0)
		return -1;

	p = strchr(dm.items[c], ' ');
	if(p != 0)
		*p = '\0';

	infobox(10, 10, calling, &s1);
	fd = dial(dm.items[c], 0, 0, &cd);
	if(fd < 0) {
		baddial[2] = dm.items[c];
		i3[0] = '\0';
		errstr(i3);
		baddial[4] = i3;
		infobox(12, 12, baddial, &s2);
		getc();
		restore(&s2);
		restore(&s1);
		return -1;
	}
	restore(&s1);
	if(c == 1)
		write(cd, "push fcall", 10);

	if(mount(fd, "/n/server", MREPL|MCREATE, "") < 0) {
		errstr(i1);
		se[1] = i1;
		se[3] = "Mounting the remote file system";
		infobox(12, 12, se, &s1);
		getc();
		restore(&s1);
		return -1;
	}
	return 0;
}

void
installfs(void)
{
	int c;
	Buffer b;
	Saved s1;
	uchar nmb[4];
	char cdrom[64], work[1024];
	char i1[128], host[45], domain[45];

	if(startip() == -1)
		return;

	if(connectfs(host, domain, sizeof(domain), fsip) == -1)
		return;

	if(cdromsel(cdrom) != 0)
		return;

	if(start9660(cdrom) != 0) {
		infobox(10, 10, no9660, &s1);
		getc();
		restore(&s1);
		return;
	}

	newwindow(&s1);
	rcp("/n/cdrom", "/n/server", 0);
	restore(&s1);

	if(getfile(&b, "/n/server/lib/ndb/local") != 0)
		return;

	for(c = 0; c < 4; c++)
		nmb[c] = ipb[c] & ipmb[c];

	snprint(work, sizeof(work),
		"ipnet=p9network ip=%d.%d.%d.%d ipmask=%s\n"
		"\tipgw=%s\n"
		"\tfs=%s.%s\n"
		"\tauth=p9auth\n",
			nmb[0], nmb[1], nmb[2], nmb[3], ipmask, ipgw, host, domain);

	replace(&b, "#INSTALL this network\n", work);

	getether(fsipb, i1);
	snprint(work, sizeof(work),
		"ip=%s ether=%s sys=%s\n"
		"\tdom=%s.%s\n"
		"\tproto=il\n",
			ipaddr, i1, host, host, domain);

	replace(&b, "#INSTALL fileserver\n", work);

	putfile(&b);
}

int
myether(char *ea)
{
	int fd, n;
	char *p, buf[1024];

	strcpy(ea, "080000000000");
	fd = open("#l/ether/0/stats", OREAD);
	if(fd < 0)
		return -1;
	n = read(fd, buf, sizeof(buf));
	if(n < 0) {
		close(fd);
		return -1;
	}
	buf[n] = '\0';
	p = strstr(buf, "addr: ");
	if(p == 0)
		return -1;

	p += 6;
	while(*p && *p != ' ' && *p != '\n') {
		if(*p != ':')
			*ea++ = *p;
		p++;
	}
	return 0;
}

char *mydeflea[] =
{
	"Could not determine the Ethernet address",
	"of this machine. After installation you must",
	"edit the /lib/ndb/local entry for the machine",
	"p9auth and enter its correct Ethernet address.",
	anykey,
	0
};

char *authid[] =
{
	"Enter the site authentication user id",
	"Example: bootes",
	"This is the user id CPU and File Servers",
	"use during authentication.",
	"",
	"Authid    :                            ",
	"",
	0,
};
char authfile[] = "/n/server/lib/ndb/auth";

char *iamacpu[] =
{
	"This system has now been configured as a CPU",
	"server.  After the next reboot you must save",
	"the authentication user id, the site key and",
	"authentication domain in the NVRAM of this",
	"machine. You will be prompted for them before",
	"the system starts.",
	"Press any key to reboot the system ...",
	0,
};

void
installcpu(void)
{
	Saved s1;
	Buffer b;
	int r, fd;
	char *p, ea[NAMELEN], work[1024], authuser[NAMELEN], xdisk[2*NAMELEN];
	char partition[NAMELEN], disk[2*NAMELEN], host[45], domain[45];

	if(startip() == -1)
		return;

	if(connectfs(host, domain, sizeof(domain), usefs) == -1)
		return;

	if(disksel(disk, 0) == 0)
		return;

	strcpy(xdisk, disk);

	authuser[0] = '\0';
	if(getname(authid, authuser, sizeof(authuser)) == 0)
		return;

	infobox(10, 10, ikernel, &s1);

	strcpy(partition, "??");
	p = strrchr(disk, '/');
	if(p != 0) {
		*p++ = '\0';
		sprint(partition, "%sboot", p);
	}

	r = copy("/n/server/386", "9pccpu", disk, partition, 0);
	restore(&s1);
	if(r == -1)
		return;

	if(getfile(&b, "/n/server/lib/ndb/local") != 0)
		return;

	if(myether(ea) == -1) {
		infobox(10, 10, mydeflea, &s1);
		getc();
		restore(&s1);
	}

	snprint(work, sizeof(work),
		"ip=%d.%d.%d.%d ether=%s sys=p9auth\n"
		"\tdom=auth.%s\n"
		"\tbootf=9pccpudisk\n"
		"\tproto=il\n",
			ipb[0], ipb[1], ipb[2], ipb[3], ea, domain);
			
	replace(&b, "#INSTALL authserver\n", work);

	putfile(&b);

	fd = create(authfile, OWRITE, 664);
	if(fd < 0) {
		warn(authfile);
		return;
	}
	fprint(fd, "hostid=%s\n\tuid=!sys uid=!adm uid=*\n", authuser);
	close(fd);

	fixplan9("/n/c/plan9/plan9.ini", xdisk);

	infobox(10, 10, iamacpu, &s1);
	reboot(1);
}

char *newdisk[] =
{
	"Place a new diskette in drive A:.",
	"It will be formatted and the contents",
	"will be lost.",
	anykey,
	0,
};

char *newdiskwait[] =
{
	"The disc will now be formatted and",
	"files copied to the floppy.",
	"Please wait ...",
	0,
};

char *mkcfg[] =
{
	"You must make a configuration file",
	"for the file server. The process can",
	"be repeated so that you can test",
	"the disc.",
	anykey,
	0,
};

char *diskdone[] =
{
	"Remove the floppy and use it to boot",
	"the file server machine. If it fails",
	"you can redo the configuration from",
	"the next menu.",
	anykey,
	0,
};

Menu fscfg =
{
	0, 0,

	"Configure File Server",
	"Done",
};

void
mkfsdisk(void)
{
	Saved s1, s2;
	int c, fd, once;
	char *av[10], cdrom[64], err[ERRLEN];

	if(cdromsel(cdrom) != 0)
		return;

	if(start9660(cdrom) != 0) {
		infobox(10, 10, no9660, &s1);
		getc();
		restore(&s1);
		return;
	}
	
	infobox(10, 9, newdisk, &s1);
	getc();
	restore(&s1);

	infobox(10, 9, newdiskwait, &s1);
	av[0] = "/bin/disk/format";
	av[1] = "-db";
	av[2] = "/n/cdrom/sys/src/boot/pc/bb";
	av[3] = "/dev/fd0disk";
	av[4] = "/n/cdrom/386/b.com";
	av[5] = "/n/cdrom/386/9pcfs";
	av[6] = "/lib/plan9.nvr";
	av[7] = 0;
	c = docmd(av, 0);
	restore(&s1);
	if(c == -1)
		return;

	once = 0;
	infobox(10, 10, mkcfg, &s1);
	getc();
	restore(&s1);
	for(;;) {
		unmount(0, "/n/a");
		fd = open("/srv/dos", ORDWR);
		if(fd < 0) {
			err[0] = '\0';
			errstr(err);
			edosfs[2] = err;
			infobox(12, 12, edosfs, &s1);
			getc();
			restore(&s1);
			return;
		}
		if(mount(fd, "/n/a", MREPL|MCREATE, "/dev/fd0disk") == -1) {
			err[0] = '\0';
			errstr(err);
			emount[2] = err;
			infobox(12, 12, emount, &s1);
			retry.choice = 0;
			c = menu(22, 18, &retry, &s2);
			restore(&s2);
			if(c == 1) {
				restore(&s1);
				return;
			}
			restore(&s1);
			continue;
		}

		configure("/n/a/plan9.ini", "fd!0!9pcfs", 0);

		if(once == 0) {
			infobox(10, 9, diskdone, &s1);
			getc();
			restore(&s1);
			once = 1;
		}

		c = menu(10, 9, &fscfg, &s1);
		restore(&s1);
		if(c == 1)
			break;
	}
}

Menu top =
{
	0, 0,

	"1. Install 3 Diskette System to local drive",
	"2. Install CD-ROM to local drive",
	"3a.Make a PC file server boot disk",
	"3b.Install CD-ROM to network file server",
	"4. Make this PC a CPU and Authentication server",
};

char n1[] = "  File System Installation";
char n2[] = "  Plan 9 (TM) Copyright (c) 1995 AT&T Bell Laboratories";

void
main(void)
{
	ulong v;
	Saved st;

	v = segattach(0, "cga", 0, 4*1024);
	if(v == -1) {
		fprint(2, "install: cannot attach cga memory: %r\n");
		exits("segattach");
	}
	cga = (uchar*)v;

	xclrscrn(17);

	xprint(0, 0, 116, "%-80s", n1);
	xprint(0, 23, 116, "%-80s", n2);

	cfd = open("/dev/consctl", OWRITE|OCEXEC);
	if(cfd < 0) {
		fprint(2, "open /dev/consctl: %r");
		exits("open");
	}
	write(cfd, "rawon", 5);
	kfd = open("/dev/cons", OREAD);
	if(kfd < 0) {
		fprint(2, "open /dev/cons: %r");
		exits("open");
	}

	for(;;) {
		switch(menu(5, 5, &top, &st)) {
		case 0:
			install4d();
			break;
		case 1:
			installcd();
			break;
		case 2:
			mkfsdisk();
			break;
		case 3:
			installfs();
			break;
		case 4:
			installcpu();
			break;
		}
		restore(&st);
	}
}

int
docmd(char **av, char *output)
{
	Waitmsg w;
	int n, fd[2];
	Saved s1, tbox;
	char *p, buf[NWIDTH], *err[10];

	if(output) {
		if(pipe(fd) < 0) {
			err[0] = "Pipe system call failed";
			goto syserr;
		}
		newwindow(&tbox);
		scroll(output);
	}

	switch(fork()) {
	case 0:
		if(output == 0) {
			close(1);	/* Not interested in idle chatter */
			close(2);
		}
		else {
			close(fd[1]);
			dup(fd[0], 1);
			dup(fd[0], 2);
		}
		exec(av[0], av);
		exits("exec failed");
	case -1:
		if(output) {
			close(fd[0]);
			close(fd[1]);
		}
		err[0] = "Fork System call failed";
	syserr:
		w.msg[0] = '\0';
		errstr(w.msg);
		err[1] = w.msg;
		err[2] = "Reboot and try again";
		err[3] = 0;
		infobox(20, 20, err, &s1);
		getc();
		restore(&s1);
		reboot(0);
		break;
	default:
		if(output) {
			close(fd[0]);
			for(;;) {
				n = read(fd[1], buf, NWIDTH);
				if(n <= 0) {
					close(fd[1]);
					break;
				}
				buf[n] = '\0';
				p = strchr(buf, '\n');
				if(p != 0)
					*p = '\0';
				scroll(buf);
			}
		}
		if(wait(&w) < 0) {
			err[0] = "Wait System call failed";
			goto syserr;
		}
		if(w.msg[0] != '\0') {
			w.msg[ERRLEN-1] = '\0';
			err[0] = "An Error occurred executing the command";
			err[1] = av[0];
			err[2] = "The error was:",
			err[3] = w.msg;
			err[4] = anykey;
			err[5] = 0;
			infobox(15, 15, err, &s1);
			getc();
			restore(&s1);
			if(output)
				restore(&tbox);
			return -1;
		}
		if(output)
			restore(&tbox);
		return 0;
	}
	if(output)
		restore(&tbox);
	return -1;
}

int
copy(char *fromdir, char *fromelem, char *todir, char *toelem, Dir *d)
{
	int f, t, n, mode, ok;
	char from[512], to[512], buf[8192];

	if(fromelem)
		sprint(from, "%s/%s", fromdir, fromelem);
	else
		strcpy(from, fromdir);

	sprint(to, "%s/%s", todir, toelem);

	if(d != 0)
		scroll(to);

	f = open(from, OREAD);
	if(f < 0) {
		warn(from);
		return -1;
	}
	if(d == 0)
		mode = 0777;
	else
		mode = d->mode|0200;
	t = create(to, OWRITE, mode);
	if(t < 0) {
		close(f);
		warn(to);
		return -1;
	}
	ok = 0;
	for(;;) {
		n = read(f, buf, sizeof(buf));
		if(n < 0) {
			warn(from);
			ok = -1;
			break;
		}
		if(n == 0)
			break;
		if(write(t, buf, n) != n) {
			warn(to);
			ok = -1;
			break;
		}
	}
	close(f);
	if(d != 0) {
		if(dirfwstat(t, d) < 0) {
			snprint(buf, NWIDTH-1, "wstat: %r");
			scroll(buf);
		}
	}
	close(t);
	return ok;
}

void
rcp(char *name, char *todir, Dir *dir)
{
	int fd, i, n;
	Dir buf[25], b;
	char file[256], newdir[256], sbuf[DIRLEN];

	if(dirstat(todir, &b) < 0){
		scroll(todir);
		fd = create(todir, OREAD, CHDIR + 0777L);
		if(fd < 0){
			warn(todir);
			return;
		}
		if(stat(name, sbuf) < 0) {
			snprint(newdir, NWIDTH-1, "stat: %r");
			scroll(newdir);
		}
		else
		if(fwstat(fd, sbuf) < 0) {
			snprint(newdir, NWIDTH-1, "wstat: %r");
			scroll(newdir);
		}
		close(fd);
	}
		
	if(dir == 0){
		dir = &buf[0];
		if(dirstat(name, dir) < 0){
			warn(name);
			return;
		}
		if((dir->mode&ISDIR) == 0) {
			copy(name, (char*)0, todir, dir->name, dir);
			return;
		}
	}
	fd = open(name, OREAD);
	if(fd < 0){
		warn(name);
		return;
	}
	while((n=dirread(fd, buf, sizeof buf)) > 0){
		n /= sizeof(Dir);
		dir = buf;
		for(i=0; i<n; i++, dir++){
			if((dir->mode&ISDIR)==0){
				copy(name, dir->name, todir, dir->name, dir);
				continue;
			}
			if(strcmp(dir->name, ".")==0 || strcmp(dir->name, "..")==0)
				continue;
			sprint(file, "%s/%s", name, dir->name);
			sprint(newdir, "%s/%s", todir, dir->name);
			if(dirstat(file, &b) < 0){
				warn(file);
				continue;
			}
			if(b.qid.path!=dir->qid.path ||
			   b.dev!=dir->dev || b.type!=dir->type)
				continue;	/* file is hidden */
			rcp(file, newdir, dir);
		}
	}
	if(n<0)
		warn("name");

	close(fd);
}

char *warni[] =
{
	"Problem Copying Files",
	0,
	"File:",
	0,
	"Press any key to continue",
	0,
};

void
warn(char *s)
{
	Saved s1;
	char err[ERRLEN];

	errstr(err);
	warni[1] = err;
	warni[3] = s;
	infobox(WLOCX+5, WLOCY+7, warni, &s1);
	getc();
	restore(&s1);
}

void
newwindow(Saved *s1)
{
	int i;

	for(i = 0; i < NLINES; i++) {
		if(window[i] == 0)
			window[i] = malloc(NWIDTH);
		memset(window[i], ' ', NWIDTH);
		window[i][NWIDTH-1] = '\0';
	}
	infobox(WLOCX, WLOCY, window, s1);
}

void
scroll(char *str)
{
	int i, l;

	for(i = 0; i < NLINES-1; i++)
		memmove(window[i], window[i+1], NWIDTH);

	memset(window[NLINES-1], ' ', NWIDTH);
	window[NLINES-1][NWIDTH-1] = '\0';
	l = strlen(str);
	if(l > NWIDTH-1)
		l = NWIDTH-1;
	strncpy(window[NLINES-1], str, l);
	window[NLINES-1][NWIDTH-1] = '\0';
	infobox(WLOCX, WLOCY, window, 0);
}
