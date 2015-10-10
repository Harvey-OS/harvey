#include <u.h>
#include <libc.h>


#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>

/* Uses fs code from cinap's realemu */

static int logfd = 2;
static int stdinmode = 0;
static int frchld[2];
static int tochld[2];
static int opencount; // This will increase, and when it goes to 0, we exit.
static Channel *reqchan;
static Channel *flushchan;

static char Ebusy[] = "device is busy";
static char Eintr[] = "interrupted";

static int Qroot, Qcons, Qconsctl;
static File *Cons, *ConsCtl;

static int
flushed(void *r)
{
	return nbrecvp(flushchan) == r;
}
 
static void
fsopen(Req *r)
{
	r->fid->file->aux = (void *)r->fid->qid.path;
	respond(r, nil);
}


static void
cpuproc(void *data)
{
	unsigned long path;
	unsigned long n;
	char *p;
	char ch;
	Req *r;
	int opval;

	threadsetname("cpuproc");
	if (chatty9p) fprint(logfd, "cpuproc started\n");

	while(r = recvp(reqchan)){
		if(flushed(r)){
			respond(r, Eintr);
			continue;
		}

		path = r->fid->qid.path;

		p = r->ifcall.data;
		n = r->ifcall.count;

		opval = ((int)r->ifcall.type<<8)|path;
		if (opval == (Tread<<8) | Qcons) {
			if (chatty9p) fprint(2, "read pipe\n");
			if (read(tochld[1], &ch, 1) > 0) {
				if (chatty9p) fprint(2, "read '%c', \n", ch);
				readbuf(r, &ch, 1);
				respond(r, nil);
				if (chatty9p) fprint(2, "read done\n");
			} else if (opval == (Twrite<<8) | Qcons) {
				if (chatty9p) fprint(2, "writepipe\n");
				write(frchld[0], p, n);
				r->ofcall.count = n;
				respond(r, nil);
				if (chatty9p) fprint(2, "wdone\n");
				break;
			}
		}
	}
}

static void
dispatch(Req *r)
{
	if(!nbsendp(reqchan, r)) {
		if (chatty9p) fprint(logfd, "reqchan was busy\n");
		respond(r, Ebusy);
	}
}

// directory reading is supported by lib9p
static void
fsread(Req *r)
{
	dispatch(r);
}

static void
fsend(Srv* srv)
{
	threadexitsall(nil);
}

static Srv fs = {
	.open=			fsopen,
	.read=			fsread,
	.write=			dispatch,
	.end=			fsend,
};

enum {
	Spec =	0xF800,		/* Unicode private space */
		PF =	Spec|0x20,	/* num pad function key */
		View =	Spec|0x00,	/* view (shift window up) */
		Shift =	Spec|0x60,
		Break =	Spec|0x61,
		Ctrl =	Spec|0x62,
		kbAlt =	Spec|0x63,
		Caps =	Spec|0x64,
		Num =	Spec|0x65,
		Middle =	Spec|0x66,
		Altgr =	Spec|0x67,
		Kmouse =	Spec|0x100,
	No =	0x00,		/* peter */

	KF =	0xF000,		/* function key (begin Unicode private space) */
		Home =	KF|13,
		Up =		KF|14,
		Pgup =		KF|15,
		Print =	KF|16,
		Left =	KF|17,
		Right =	KF|18,
		End =	KF|24,
		Down =	View,
		Pgdown =	KF|19,
		Ins =	KF|20,
		Del =	0x7F,
		Scroll =	KF|21,
};

typedef struct Keybscan Keybscan;
struct Keybscan {
	int esc1;
	int esc2;
	int alt;
	int altgr;
	int caps;
	int ctl;
	int num;
	int shift;
	int collecting;
	int nk;
	Rune kc[5];
	int buttons;
};

static Keybscan kbscan;

static Rune kbtab[256] =
{
[0x00]	No,	'\x1b',	'1',	'2',	'3',	'4',	'5',	'6',
[0x08]	'7',	'8',	'9',	'0',	'-',	'=',	'\b',	'\t',
[0x10]	'q',	'w',	'e',	'r',	't',	'y',	'u',	'i',
[0x18]	'o',	'p',	'[',	']',	'\n',	Ctrl,	'a',	's',
[0x20]	'd',	'f',	'g',	'h',	'j',	'k',	'l',	';',
[0x28]	'\'',	'`',	Shift,	'\\',	'z',	'x',	'c',	'v',
[0x30]	'b',	'n',	'm',	',',	'.',	'/',	Shift,	'*',
[0x38]	kbAlt,	' ',	Ctrl,	KF|1,	KF|2,	KF|3,	KF|4,	KF|5,
[0x40]	KF|6,	KF|7,	KF|8,	KF|9,	KF|10,	Num,	Scroll,	'7',
[0x48]	'8',	'9',	'-',	'4',	'5',	'6',	'+',	'1',
[0x50]	'2',	'3',	'0',	'.',	No,	No,	No,	KF|11,
[0x58]	KF|12,	No,	No,	No,	No,	No,	No,	No,
[0x60]	No,	No,	No,	No,	No,	No,	No,	No,
[0x68]	No,	No,	No,	No,	No,	No,	No,	No,
[0x70]	No,	No,	No,	No,	No,	No,	No,	No,
[0x78]	No,	View,	No,	Up,	No,	No,	No,	No,
};

static Rune kbtabshift[256] =
{
[0x00]	No,	'\x1b',	'!',	'@',	'#',	'$',	'%',	'^',
[0x08]	'&',	'*',	'(',	')',	'_',	'+',	'\b',	'\t',
[0x10]	'Q',	'W',	'E',	'R',	'T',	'Y',	'U',	'I',
[0x18]	'O',	'P',	'{',	'}',	'\n',	Ctrl,	'A',	'S',
[0x20]	'D',	'F',	'G',	'H',	'J',	'K',	'L',	':',
[0x28]	'"',	'~',	Shift,	'|',	'Z',	'X',	'C',	'V',
[0x30]	'B',	'N',	'M',	'<',	'>',	'?',	Shift,	'*',
[0x38]	kbAlt,	' ',	Ctrl,	KF|1,	KF|2,	KF|3,	KF|4,	KF|5,
[0x40]	KF|6,	KF|7,	KF|8,	KF|9,	KF|10,	Num,	Scroll,	'7',
[0x48]	'8',	'9',	'-',	'4',	'5',	'6',	'+',	'1',
[0x50]	'2',	'3',	'0',	'.',	No,	No,	No,	KF|11,
[0x58]	KF|12,	No,	No,	No,	No,	No,	No,	No,
[0x60]	No,	No,	No,	No,	No,	No,	No,	No,
[0x68]	No,	No,	No,	No,	No,	No,	No,	No,
[0x70]	No,	No,	No,	No,	No,	No,	No,	No,
[0x78]	No,	Up,	No,	Up,	No,	No,	No,	No,
};

static Rune kbtabesc1[256] =
{
[0x00]	No,	No,	No,	No,	No,	No,	No,	No,
[0x08]	No,	No,	No,	No,	No,	No,	No,	No,
[0x10]	No,	No,	No,	No,	No,	No,	No,	No,
[0x18]	No,	No,	No,	No,	'\n',	Ctrl,	No,	No,
[0x20]	No,	No,	No,	No,	No,	No,	No,	No,
[0x28]	No,	No,	Shift,	No,	No,	No,	No,	No,
[0x30]	No,	No,	No,	No,	No,	'/',	No,	Print,
[0x38]	Altgr,	No,	No,	No,	No,	No,	No,	No,
[0x40]	No,	No,	No,	No,	No,	No,	Break,	Home,
[0x48]	Up,	Pgup,	No,	Left,	No,	Right,	No,	End,
[0x50]	Down,	Pgdown,	Ins,	Del,	No,	No,	No,	No,
[0x58]	No,	No,	No,	No,	No,	No,	No,	No,
[0x60]	No,	No,	No,	No,	No,	No,	No,	No,
[0x68]	No,	No,	No,	No,	No,	No,	No,	No,
[0x70]	No,	No,	No,	No,	No,	No,	No,	No,
[0x78]	No,	Up,	No,	No,	No,	No,	No,	No,
};

static Rune kbtabaltgr[256] =
{
[0x00]	No,	No,	No,	No,	No,	No,	No,	No,
[0x08]	No,	No,	No,	No,	No,	No,	No,	No,
[0x10]	No,	No,	No,	No,	No,	No,	No,	No,
[0x18]	No,	No,	No,	No,	'\n',	Ctrl,	No,	No,
[0x20]	No,	No,	No,	No,	No,	No,	No,	No,
[0x28]	No,	No,	Shift,	No,	No,	No,	No,	No,
[0x30]	No,	No,	No,	No,	No,	'/',	No,	Print,
[0x38]	Altgr,	No,	No,	No,	No,	No,	No,	No,
[0x40]	No,	No,	No,	No,	No,	No,	Break,	Home,
[0x48]	Up,	Pgup,	No,	Left,	No,	Right,	No,	End,
[0x50]	Down,	Pgdown,	Ins,	Del,	No,	No,	No,	No,
[0x58]	No,	No,	No,	No,	No,	No,	No,	No,
[0x60]	No,	No,	No,	No,	No,	No,	No,	No,
[0x68]	No,	No,	No,	No,	No,	No,	No,	No,
[0x70]	No,	No,	No,	No,	No,	No,	No,	No,
[0x78]	No,	Up,	No,	No,	No,	No,	No,	No,
};

static Rune kbtabctrl[256] =
{
[0x00]	No,	'\x1b',	'\x11',	'\x12',	'\x13',	'\x14',	'\x15',	'\x16',
[0x08]	'\x17',	'\x18',	'\x19',	'\x10',	'\n',	'\x1d',	'\b',	'\t',
[0x10]	'\x11',	'\x17',	'\x05',	'\x12',	'\x14',	'\x19',	'\x15',	'\t',
[0x18]	'\x0f',	'\x10',	'\x1b',	'\x1d',	'\n',	Ctrl,	'\x01',	'\x13',
[0x20]	'\x04',	'\x06',	'\x07',	'\b',	'\n',	'\x0b',	'\x0c',	'\x1b',
[0x28]	'\x07',	No,	Shift,	'\x1c',	'\x1a',	'\x18',	'\x03',	'\x16',
[0x30]	'\x02',	'\x0e',	'\n',	'\x0c',	'\x0e',	'\x0f',	Shift,	'\n',
[0x38]	kbAlt,	No,	Ctrl,	'\x05',	'\x06',	'\x07',	'\x04',	'\x05',
[0x40]	'\x06',	'\x07',	'\x0c',	'\n',	'\x0e',	'\x05',	'\x06',	'\x17',
[0x48]	'\x18',	'\x19',	'\n',	'\x14',	'\x15',	'\x16',	'\x0b',	'\x11',
[0x50]	'\x12',	'\x13',	'\x10',	'\x0e',	No,	No,	No,	'\x0f',
[0x58]	'\x0c',	No,	No,	No,	No,	No,	No,	No,
[0x60]	No,	No,	No,	No,	No,	No,	No,	No,
[0x68]	No,	No,	No,	No,	No,	No,	No,	No,
[0x70]	No,	No,	No,	No,	No,	No,	No,	No,
[0x78]	No,	'\x07',	No,	'\b',	No,	No,	No,	No,
};

int
keybscan(uint8_t code, char *out, int len)
{
	Rune c;
	int keyup;
	int off;

if (chatty9p) fprint(2, "kbscan: code 0x%x out %p len %d\n", code, out, len);
	c = code;
	off = 0;
	if(len < 16){
		if (chatty9p) fprint(2, "keybscan: input buffer too short to be safe\n");
		return -1;
	}
	if(c == 0xe0){
		kbscan.esc1 = 1;
		return off;
	} else if(c == 0xe1){
		kbscan.esc2 = 2;
		return off;
	}

	keyup = c&0x80;
	c &= 0x7f;
	if(c > sizeof kbtab){
		c |= keyup;
		if(c != 0xFF)	/* these come fairly often: CAPSLOCK U Y */
			if (chatty9p) fprint(2, "unknown key %ux\n", c);
		return off;
	}

	if(kbscan.esc1){
		c = kbtabesc1[c];
		kbscan.esc1 = 0;
	} else if(kbscan.esc2){
		kbscan.esc2--;
		return off;
	} else if(kbscan.shift)
		c = kbtabshift[c];
	else if(kbscan.altgr)
		c = kbtabaltgr[c];
	else if(kbscan.ctl)
		c = kbtabctrl[c];
	else
		c = kbtab[c];

	if(kbscan.caps && c<='z' && c>='a')
		c += 'A' - 'a';

	/*
	 *  keyup only important for shifts
	 */
	if(keyup){
		switch(c){
		case kbAlt:
			kbscan.alt = 0;
			break;
		case Shift:
			kbscan.shift = 0;
			/*mouseshifted = 0;*/
			break;
		case Ctrl:
			kbscan.ctl = 0;
			break;
		case Altgr:
			kbscan.altgr = 0;
			break;
		case Kmouse|1:
		case Kmouse|2:
		case Kmouse|3:
		case Kmouse|4:
		case Kmouse|5:
			kbscan.buttons &= ~(1<<(c-Kmouse-1));
			/*if(kbdmouse)kbdmouse(kbscan.buttons);*/
			break;
		}
		return off;
	}

	/*
	 *  normal character
	 */
	if(!(c & (Spec|KF))){
		off += runetochar(out+off, &c);
		return off;
	} else {
		switch(c){
		case Caps:
			kbscan.caps ^= 1;
			return off;
		case Num:
			kbscan.num ^= 1;
			return off;
		case Shift:
			kbscan.shift = 1;
			/*mouseshifted = 1;*/
			return off;
		case kbAlt:
			kbscan.alt = 1;
			/*
			 * VMware and Qemu use Ctl-Alt as the key combination
			 * to make the VM give up keyboard and mouse focus.
			 * This has the unfortunate side effect that when you
			 * come back into focus, Plan 9 thinks you want to type
			 * a compose sequence (you just typed alt). 
			 *
			 * As a clumsy hack around this, we look for ctl-alt
			 * and don't treat it as the start of a compose sequence.
			 */
			return off;
		case Ctrl:
			kbscan.ctl = 1;
			return off;
		case Altgr:
			kbscan.altgr = 1;
			return off;
		case Kmouse|1:
		case Kmouse|2:
		case Kmouse|3:
		case Kmouse|4:
		case Kmouse|5:
			kbscan.buttons |= 1<<(c-Kmouse-1);
			/*if(kbdmouse)kbdmouse(kbscan.buttons);*/
			return off;
		}
	}

	off += runetochar(out+off, &c);
	return off;

}

enum {
	Black		= 0x00,
	Blue		= 0x01,
	Green		= 0x02,
	Cyan		= 0x03,
	Red		= 0x04,
	Magenta		= 0x05,
	Brown		= 0x06,
	Grey		= 0x07,

	Bright 		= 0x08,
	Blinking	= 0x80,

	kbAttr		= (Black<<4)|Grey,	/* (background<<4)|foreground */
	Cursor = (Grey<<4)|Black,
};

enum {
	Width		= 80*2,
	Height		= 25,
};

int cgapos;
uint8_t CGA[16384];

static void
cgaputc(int c)
{
	int i;
	uint8_t *cga, *p;

	cga = CGA;

	cga[cgapos] = ' ';
	cga[cgapos+1] = kbAttr;

	switch(c){
	case '\r':
		break;
	case '\n':
		cgapos = cgapos/Width;
		cgapos = (cgapos+1)*Width;
		break;
	case '\t':
		i = 8 - ((cgapos/2)&7);
		while(i-- > 0)
			cgaputc(' ');
		break;
	case '\b':
		if(cgapos >= 2)
			cgapos -= 2;
		break;
	default:
		cga[cgapos++] = c;
		cga[cgapos++] = kbAttr;
		break;
	}

	if(cgapos >= (Width*Height)){
		memmove(cga, &cga[Width], Width*(Height-1));
		p = &cga[Width*(Height-1)];
		for(i = 0; i < Width/2; i++){
			*p++ = ' ';
			*p++ = kbAttr;
		}

		cgapos -= Width;
	}

	cga[cgapos] = ' ';
	cga[cgapos+1] = Cursor;
}

static void
cgawrite(uint8_t *buf, int len)
{
	int i;
	for(i = 0; i < len; i++)
		cgaputc(buf[i]);
}

static void
cgaflush(int fd)
{
	pwrite(fd, CGA, Width*Height, 0);
}

void
fsdestroyfile(File *f)
{
	
	if(chatty9p)fprint(logfd, "%s:f %p,  aux %p\n", __func__, f, f->aux);

	if (f->aux != (void *)Qcons)
		return;
	adec(&opencount);
	if (opencount == 0)
		threadexitsall(nil);
}


void
threadmain(int argc, char *argv[])
{
	int fd0, fd1, fd2;
	int ps2fd = 0, cgafd = 1;
	int pid, wpid;
	int i, n, off, len;
	static uint8_t ibuf[32];
	static uint8_t buf[8*sizeof ibuf];
	char *argv0 = argv[0];

	chatty9p++;
	ARGBEGIN{
	case 'd':
		chatty9p++;
		break;
	case 's':
		stdinmode = 1;
		break;
	default:
		exits("usage");
	}ARGEND;

	fs.tree = alloctree(nil, nil, DMDIR|0777, fsdestroyfile);
	Qroot = fs.tree->root->qid.path;
	Cons = createfile(fs.tree->root, "cons", "", 0666, &opencount);
	Qcons = Cons->qid.path;
	ConsCtl = createfile(fs.tree->root, "consctl", "", 0666, nil);
	Qconsctl = ConsCtl->qid.path;

	if (chatty9p) fprint(2, "Try to fork %s\n", argv[0]);
	if (! stdinmode) {
		if((ps2fd = open("#P/ps2keyb", OREAD)) == -1){
			errstr(buf, sizeof buf);
			fprint(2, "open #P/ps2keyb: %s\n", buf);
			exits("open");
		}

		if((cgafd = open("#P/cgamem", OWRITE)) == -1){
			errstr(buf, sizeof buf);
			fprint(2, "open #P/cgamem: %s\n", buf);
			exits("open");
		}
	}

	pipe(frchld);
	pipe(tochld);
	reqchan = chancreate(sizeof(Req*), 8);
	flushchan = chancreate(sizeof(Req*), 8);
	procrfork(cpuproc, nil, 16*1024, RFNAMEG|RFNOTEG);
	threadpostmountsrv(&fs, nil, "/dev", MBEFORE);

	// TODO: Wait for the last close of /dev/cons and exit. This will be a bit more work.
	// rfnameg to put command into a new pgid (in case we are init)
	switch(pid = rfork(RFPROC|RFCFDG|RFNOTEG|RFNAMEG)){ 
	case -1:
		exits("fork child");
	case 0:
		fd0 = open("/dev/cons", OREAD);
		fd1 = open("/dev/cons", OWRITE);
		fd2 = dup(fd1, 2);
		if ((fd0 != 0) || (fd1 != 1) || (fd2 != 2)) {
			fprint(fd2, "WTF? open of clean gave fds of %d, %d, %d\n", fd0, fd1, fd2);
			exits("bad FDs");
		}
		exec(argv[0], argv);
		fprint(2, "exec of %s failed: %r", argv[0]);
		exits(smprint("EXEC FAILED: %r"));
	default:
		break;
	}

	switch(wpid = rfork(RFPROC|RFFDG)){
	case -1:
		exits("rfork");
	case 0:
		off = 0;
		close(cgafd);
		close(frchld[1]);
		while((len = read(ps2fd, ibuf, sizeof ibuf)) > 0){
if (chatty9p) fprint(2, "psfd reads %d\n", len);
			for(i = 0; i < len; i++){
if (chatty9p) fprint(2, " %d\n", i);
if (chatty9p) fprint(2, " ibuf[i] 0x%x buf %p buf + off %p sizeof buf - off %d\n", ibuf[i], buf, buf+off, sizeof buf - off);
				if((n = keybscan(ibuf[i], buf+off, sizeof buf - off)) == -1){
					//fprint(2, "keybscan -1\n");
					break;
				}
if (chatty9p) fprint(2, "n is %d so off will be %d\n", n, off + n);
				off += n;
			}
if (chatty9p) fprint(2, "off is now %d\n", off);
			if(off > 0){
if (chatty9p) fprint(2, "write %d, %p, %d\n", tochld[0], buf, off);
				write(tochld[0], buf, off);
				off = 0;
			}
		}
if (chatty9p) fprint(2, "psfd read returned <= 0\n");
		postnote(PNPROC, getppid(), "interrupt");
		exits(nil);
	default:
		close(0);
		close(ps2fd);
		close(tochld[0]);
if (chatty9p) fprint(2, "read from child on %d\n", frchld[1]);
		while((n = read(frchld[1], buf, sizeof buf)) >= 0){
			if (stdinmode) {
				if (write(cgafd, buf, n) < n)
					break;
				continue;
			}
			Dir *dirp;
			if(n > 0){
				cgawrite(buf, n);
			}
			if((dirp = dirfstat(frchld[1])) != nil){
				if(dirp->length == 0)
					cgaflush(cgafd);
				free(dirp);
			} else {
				cgaflush(cgafd);
			}
		}
if (chatty9p) fprint(2, "read returns %d\n", n);
		if(n < 0){
			//fprint(2, "read: %r\n");
			exits("read");
		}
		close(cgafd);
		postnote(PNPROC, wpid, "interrupt");
		postnote(PNPROC, pid, "interrupt");
		waitpid();
		waitpid();
	}

	exits(nil);
}
