#include <u.h>
#include <libc.h>
#include "dat.h"

uchar*	cga;
char	kbdc[10];
int	kbdcnt;
int	cfd;
int	kfd;
char*	window[NLINES+1];

int	chkinstall(void);
void	install(void);
void	warn(char*);
void	scroll(char*);
void	fsiconfig(char*);

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

char n1[] = "  System Installation & Configuration";
char n2[] = "  Plan 9 (TM) Copyright (c) 1995 AT&T Bell Laboratories";
char *done[] =
{
	"The first part of the Installation is",
	"complete. Remove the  boot floppy and",
	"press a key. The system should reboot",
	"automatically.",
	"",
	"When you return to DOS, restart Plan 9",
	"by typing PLAN9\\B at the DOS prompt.",
	0,
};


char*
bootfile(void)
{
	int fd;
	char *bf;

	bf = "hd!0!/plan9/9dos";

	fd = open("#s/dos", ORDWR);
	if(fd >= 0) {
		mount(fd, "/n/dos", MREPL, "/dev/sd0disk");
		close(fd);
		if(access("/n/dos/plan9/9dos", 0) == 0)
			bf = "sd!0!/plan9/9dos";
		unmount(0, "/n/dos");
	}
	return bf;
}

void
main(void)
{
	ulong v;

	v = segattach(0, "cga", 0, 4*1024);
	if(v == -1) {
		fprint(2, "install: cannot attach CGA memory: %r\n");
		exits("segattach");
	}
	cga = (uchar*)v;

	xclrscrn(17);

	xprint(0, 0, 116, "%-80s", n1);
	xprint(0, 23, 116, "%-80s", n2);

	cfd = open("/dev/consctl", OWRITE);
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

	if(chkinstall() == 0)
		install();

	configure("/n/c/plan9/plan9.ini", bootfile(), "/plan9");
	infobox(5, 5, done, 0);
	reboot(1);
}

char *reconinfo[] =
{
	"Pick which configuration will boot by default.",
	"This can be overridden by supplying an argument",
	"to C:\\PLAN9\\B.",
	0,
};

Menu rcb =
{
	0, 0,

	"File System Installation",
	"An active Plan 9 System",
};

char *cdi[] =
{
	"Choose the boot partition to mark as the",
	"active system.",
	0,
};

char *nodisks[] =
{
	"No disks have an active Plan 9 partition table",
	anykey,
	0,
};

int
disksel(char *dev)
{
	Menu ld;
	int c, l, i;
	Saved s1, s2;
	char drives[NMENU][32];

	dev[0] = '\0';

	l = 0;
	memset(&ld, 0, sizeof(Menu));
	for(i = 0; i < 4; i++) {
		sprint(drives[l], "/dev/hd%dboot", i);
		if(access(drives[l], 0) == 0) {
			sprint(drives[l], "/dev/hd%d", i);
			ld.items[l++] = drives[l];
		}
	}
	for(i = 0; i < 8; i++) {
		sprint(drives[l], "/dev/sd%dboot", i);
		if(access(drives[l], 0) == 0) {
			sprint(drives[l], "/dev/sd%d", i);
			ld.items[l++] = drives[l];
		}
	}

	if(l == 0) {
		infobox(10, 10, nodisks, &s1);
		getc();
		restore(&s1);
		return 0;
	}

	infobox(10, 10, cdi, &s1);
	c = menu(14, 13, &ld, &s2);
	restore(&s2);
	restore(&s1);
	if(ld.selected == 0)
		return 0;

	strcpy(dev, drives[c]);
	return 1;
}

char *recondone[] =
{
	"Reconfiguration complete. Remove the",
	"boot floppy and press any key to reboot,",
	"allowing the new configuration to come",
	"into effect.",
	"",
	"When you return to DOS, restart Plan 9",
	"by typing PLAN9\\B at the DOS prompt.",
	0,
};

void
reconfigure(void)
{
	int c;
	Saved s1, s2;
	char disk[3*NAMELEN];

	configure("/n/c/plan9/plan9.ini", bootfile(), "/plan9");

	infobox(10, 10, reconinfo, &s1);
	c = menu(14, 14, &rcb, &s2);
	restore(&s2);
	restore(&s1);
	if(c == 1 && disksel(disk) == 1)
		fixplan9("/n/c/plan9/plan9.ini", disk);

	infobox(5, 5, recondone, 0);
	reboot(1);
}

char *p9exists[] =
{
	"Plan 9 is already installed on",
	"the DOS file system in C:\\plan9",
	0,
};

Menu ask =
{
	1, 0,

	"Remove the installation",
	"Reconfigure the system",
	"File System Installation",
};

int
docmd(char **av, char*)
{
	Waitmsg w;
	Saved s1;
	char *err[10];

	switch(fork()) {
	case 0:
		exec(av[0], av);
		exits("exec failed");
	case -1:
		err[0] = "Fork System call failed";
	syserr:
		errstr(w.msg);
		err[1] = w.msg;
		err[2] = "Reboot and try again";
		err[3] = 0;
		infobox(20, 20, err, &s1);
		getc();
		restore(&s1);
		break;
	default:
		if(wait(&w) < 0) {
			err[0] = "Wait System call failed";
			goto syserr;
		}
		if(w.msg[0] != '\0') {
			err[0] = "An Error occurred executing a Command";
			err[1] = av[0];
			err[2] = w.msg;
			err[3] = "Reboot and try again";
			err[4] = 0;
			infobox(20, 20, err, &s1);
			getc();
			restore(&s1);
			return 0;
		}
	}
	return -1;
}

char *remit[] =
{
	"Removing old installation",
	"Please wait ...",
	0
};

int
chkinstall(void)
{
	int c;
	char *av[5];
	Saved s1, s2, s3;

	if(access("/n/c/plan9", OREAD) != 0)
		return 0;

	infobox(10, 5, p9exists, &s1);
	for(;;) {
		c = menu(15, 10, &ask, &s2);
		restore(&s2);
		switch(c) {
		case 0:
			infobox(18, 12, remit, &s3);
			av[0] = "/bin/rm";
			av[1] = "-rf";
			av[2] = "/n/c/plan9";
			av[3] = 0;
			docmd(av, 0);
			restore(&s3);
			restore(&s1);
			return 0;
		case 1:
			if(ask.selected) {
				restore(&s1);
				reconfigure();
			}
			break;
		case 2:
			fsiconfig("/n/c/plan9/plan9.ini");
			infobox(5, 5, recondone, 0);
			reboot(1);
		}
	}
	return 0;
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
	close(t);
	return ok;
}

void
rcp(char *name, char *todir, Dir *dir)
{
	int fd, i, n;
	Dir buf[25], b;
	char file[256], newdir[256];

	if(dirstat(todir, &b) < 0){
		scroll(todir);
		fd = create(todir, OREAD, CHDIR + 0777L);
		if(fd < 0){
			warn(todir);
			return;
		}
		close(fd);
	}

	if(strncmp(name, "/n/c", 4) == 0)
		return;
		
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

char *info[] =
{
	"System installation will create",
	"a new  directory  C:\\plan9  and",
	"copy about 1.44 Mbytes onto the",
	"C: drive",
	0
};

char *aborted[] =
{
	"System installation was aborted",
	"Press any key to reboot",
	0
};

Menu proceed =
{
	0, 0,
	"Proceed",
	"Abort",
};

char *warni[] =
{
	"Problem Copying Files",
	0,
	"File:",
	0,
	anykey,
	0,
};

void
warn(char *s)
{
	Saved s1;
	char err[ERRLEN];

	err[0] = '\0';
	errstr(err);
	warni[1] = err;
	warni[3] = s;
	infobox(WLOCX+5, WLOCY+7, warni, &s1);
	getc();
	restore(&s1);
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

char *devconf[] =
{
	"Plan 9 is now installed on C:\\plan9.",
	"You must now configure the devices.",
	"Look at plan9.ini(8) in the manual",
	"to help understand the choices",
	" ",
	anykey,
	0,
};

char *scsionly[] =
{
	"Install cannot proceed because it cannot find the",
	"primary DOS partition."
	"",
	"If your primary DOS partition is on a SCSI device,",
	"you must first configure the SCSI controller and",
	"then reboot to continue the installation. To do",
	"this, first make a writable copy of diskette 1",
	"using the DOS DISKCOPY command. Then reboot using",
	"the new copy and you will be able to use the",
	"`Configure SCSI' option.",
	"",
	"If you think your primary DOS partition is on an",
	"IDE drive, Plan 9 has failed to find it.",
	"Without modifying your hardware configuration",
	"install is unable to proceed.",
	0,
};

Menu cfgscsi =
{
	0, 0,

	"Configure SCSI",
	"Reboot",
};

void
checkdos(void)
{
	int c;
	Saved s1, s2;

	if(access("/dev/hd0disk", 0) == 0)
		return;
	if(access("/dev/sd0disk", 0) == 0)
		return;

	infobox(8, 3, scsionly, &s1);
	c = menu(27, 19, &cfgscsi, &s2);
	restore(&s2);
	switch(c) {
	case 0:
		restore(&s1);
		scsiini();
		break;
	case 1:
		reboot(0);
	}
	restore(&s1);
}

void
install(void)
{
	int i;
	Saved s1, s2;

	checkdos();

	infobox(5, 5, info, &s1);
	i = menu(10, 10, &proceed, &s2);
	restore(&s2);
	restore(&s1);
	if(i == 1) {
		infobox(5, 5, aborted, 0);
		reboot(1);
	}

	for(i = 0; i < NLINES; i++) {
		window[i] = malloc(NWIDTH);
		memset(window[i], ' ', NWIDTH);
		window[i][NWIDTH-1] = '\0';
	}

	infobox(WLOCX, WLOCY, window, &s1);

	rcp("/n/a", "/n/c/plan9", 0);

	/*
	 * Set up for build script
	 */
	if(copy("/n/a/rc/bin", "cpurcdos", "/n/c/plan9/rc/bin", "cpurc", 0) != 0)
		warn("/n/c/plan9/rc/bin/cpurc");
	if(copy("/n/a/lib", "namesdos", "/n/c/plan9/lib", "namespace", 0) != 0)
		warn("/n/c/plan9/lib/namespace");

	infobox(10, 14, devconf, &s2);
	getc();
	restore(&s2);

	restore(&s1);

	for(i = 0; i < NLINES; i++)
		free(window[i]);
}

void
fsiconfig(char *inifile)
{
	int fd, n;
	char *p, *q, buf[8192], boot[128];

	fd = open(inifile, OREAD);
	if(fd < 0) {
		warn(inifile);
		return;
	}
	n = read(fd, buf, sizeof(buf));
	close(fd);
	if(n < 0) {
		warn(inifile);
		return;
	}
	buf[n] = '\0';

	p = strstr(buf, "rootdir=");
	if(p != 0) {
		q = strchr(p, '\n');
		if(q != 0) {
			q++;
			memmove(p, q, (buf+n) - q);
			n -= q-p;
			buf[n] = '\0';
		}
	}	
	p = strstr(buf, "bootfile=");
	if(p != 0) {
		q = strchr(p, '\n');
		if(q != 0) {
			q++;
			memmove(p, q, (buf+n) - q);
			n -= q-p;
			buf[n] = '\0';
		}
	}

	sprint(boot, "rootdir=/plan9\r\nbootfile=%s\r\n", bootfile());
	strcat(buf, boot);

	fd = create(inifile, OWRITE, 0664);
	if(fd < 0) {
		warn(inifile);
		return;
	}
	n = strlen(buf);
	if(write(fd, buf, n) != n)
		warn(inifile);
	close(fd);
}
