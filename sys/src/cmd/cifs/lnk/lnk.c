#include <u.h>
#include <libc.h>
#include <bio.h>

enum {
	/* Shortcut flags */
	FLidlist	= 0x01,		/* The shell item id list is present */
	FLfiledir	= 0x02,		/* Points to a file or directory */
	FLdesc		= 0x04,		/* Has a description string */
	FLrelative	= 0x08,		/* Has a relative path string */
	FLcwd		= 0x10,		/* Has a working directory */
	FLargs		= 0x20,		/* Has command line arguments */
	FLicon		= 0x40,		/* Has a custom icon */

	/* Volume flags */
	VOLlocal	= 0x1,
	VOLnetwork	= 0x2,
};

static char magic[] = {
	0x4c, 0x00, 0x00, 0x00,		/* shortcut magic */
	0x01, 0x14, 0x02, 0x00, 0x00,	/* 16 byte GUID */
	0x00, 0x00, 0x00, 0xc0, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x46
};

typedef struct {
	int	n;
	char	*s;
} Strtab;

Strtab attrstr[] = {
	{ 0x1,		"read only" },
	{ 0x2,		"hidden" },
	{ 0x4,		"system file" },
	{ 0x8,		"volume label" },
	{ 0x10,		"directory" },
	{ 0x20,		"archive" },
	{ 0x40,		"encrypted" },
	{ 0x80,		"normal" },
	{ 0x100,	"temporary" },
	{ 0x200,	"sparse file" },
	{ 0x400,	"reparse point data" },
	{ 0x800,	"compressed" },
	{ 0x1000,	"offline" },
};

Strtab showstr[] = {
	{ 1,	"normal" },		/* SW_SHOWNORMAL */
	{ 2,	"minimised" },		/* SW_SHOWMAXIMISED */
	{ 3,	"maximised" },		/* SW_SHOWMINIMISED */
};

Strtab Hotkey[] = {
	{ 1,	"shift" },
	{ 2,	"ctrl" },
	{ 4,	"alt" },
	{ 8,	"ext" }
};

char *disktype[] = {
	"unknown",
	"no-root-dir",
	"removable-disk",
	"fixed-disk",
	"network",
	"cdrom",
	"ram-disk"
};

char *
tofwd(char *s)
{
	char *p;

	for (p = s; *p; p++)
		if (*p == '\\')
			*p = '/';
	return s;
}


char *
gstr(Biobuf *bp)
{
	char *p;

	if ((p = Brdstr(bp, '\0', 0)) == nil)
		sysfatal("read str failed - %r");
	return p;
}

int
gmem(Biobuf *bp, void *v, int n)
{
	if (Bread(bp, v, n) != n)
		sysfatal("read n=%d failed - %r", n);
	return n;
}

int
g32(Biobuf *bp)
{
	int i, n, val;

	val = 0;
	for (i = 0; i < 4; i++){
		if ((n = Bgetc(bp)) == -1)
			sysfatal("read 32 failed - %r");
		val |= n << (i*8);
	}
	return val;
}

int
g16(Biobuf *bp)
{
	int i, n, val;

	val = 0;
	for (i = 0; i < 2; i++){
		if ((n = Bgetc(bp)) == -1)
			sysfatal("read 16 failed - %r");
		val |= n << (i*8);
	}
	return val;
}

long
gvtime(Biobuf *bp)
{
	uvlong vl;
	ulong l1, l2;

	l1 = g32(bp);
	l2 = g32(bp);
	vl = l1 | (uvlong)l2 << 32;

	vl /= 10000000LL;
	vl -= 11644473600LL;
	return (long)vl;
}

char *
lenstr(Biobuf *bp)
{
	int len;
	Rune r;
	char *p, *buf;

	len = g16(bp);			/* string len */
	if ((buf = malloc((len+1) * UTFmax)) == nil)
		sysfatal("no memory\n");
	p = buf;
	while(len-- > 0){
		r = g16(bp);
		p += runetochar(p, &r);
	}
	*p = 0;
	return buf;
}

void
locvol(Biobuf *bp)
{
	char *label;
	int serial, base, off;

	base = Boffset(bp);
	g32(bp);			/* reclen */
	print("local\n");
	print("\tdestination: %s\n", disktype[g32(bp)]);
	serial = g32(bp);
	print("\tvolume-serial: %04x-%04x\n", serial/0x10000, serial%0x10000);
	off = g32(bp);
	Bseek(bp, base+off, 0);
	label = gstr(bp);
	print("\tvolume-label: %s\n", label);
	free(label);
}

void
netvol(Biobuf *bp)
{
	char *share;
	int base, off, type, spec, code;

	base = Boffset(bp);
	g32(bp);		/* reclen */
	type = g32(bp);		/* unknown, 2 for MS nets */
	off = g32(bp);
	spec = g32(bp);		/* unknown, always 0 */
	code = g32(bp);		/* unknown, always 0x2000 */
	print("remote\n");
	print("\tspecifiers: %x %x %x\n", type, spec, code);
	Bseek(bp, base+off, 0);
	share = gstr(bp);
	print("\tshare: %s\n", share);
	free(share);
}

void
fileloc(Biobuf *bp)
{
	char *p;
	int flags, base, voloff, baseoff, netoff, tailoff, len;

	base = Boffset(bp);
	len = g32(bp);
	g32(bp);		/* offset of 1st byte after this structure */
	flags = g32(bp);	/* flags (d0 = local, d1 = network share) */
	voloff = g32(bp);	/* offset to local volume */
	baseoff = g32(bp);	/* offset to base of path on local system */
	netoff = g32(bp);	/* offset of network volume info */
	tailoff = g32(bp);	/* offset of path tail */

	if (flags & VOLlocal){
		Bseek(bp, base+voloff, 0);
		locvol(bp);
		Bseek(bp, base+baseoff, 0);
		p = gstr(bp);
		print("path-base: %s\n", p);
	}
	if (flags & VOLnetwork){
		Bseek(bp, base+netoff, 0);
		netvol(bp);
	}
	Bseek(bp, base+tailoff, 0);
	p = gstr(bp);
	print("path-tail: %s\n", p);
	Bseek(bp, base+len, 0);
}

int
dump(char *file)
{
	Biobuf *bp;
	char tmp[20];
	int i, len, flags, attrs, hotkey, iconidx, showwnd;

	if ((bp = Bopen(file, OREAD)) == nil)
		sysfatal("%s - cannot open", file);

	gmem(bp, tmp, 20);	/* magic and GUID */
	if (memcmp(tmp, magic, sizeof(magic)) != 0)
		sysfatal("%s - bad magic", file);

	print("target\n");
	flags = g32(bp);
	attrs = g32(bp);
	print("\tattributes: ");
	for (i = 0; i < nelem(attrstr); i++)
		if (attrstr[i].n & attrs)
			print("%s ", attrstr[i].s);
	print("\n");

	print("\tcreated: %s", ctime(gvtime(bp)));
	print("\tmodified: %s", ctime(gvtime(bp)));
	print("\taccessed: %s", ctime(gvtime(bp)));
	print("\tlength: %ud\n", g32(bp));

	iconidx = g32(bp);
	print("icon-index: %d\n", iconidx);
	showwnd = g32(bp);
	print("show-window: ");
	for (i = 0; i < nelem(showstr); i++)
		if (showstr[i].n == showwnd)
			print("%s ", showstr[i].s);
	print("\n");

	hotkey = g32(bp);
	print("hotkey: ");
	for (i = 0; i < nelem(showstr); i++)
		if (showstr[i].n & ((hotkey >> 8) & 0xff))
			print("%s+", showstr[i].s);
	print("%d\n", hotkey & 0xff);
	g32(bp);	/* unknown, zero */
	g32(bp);	/* unknown, zero */

	if (flags & FLidlist){
		len = g16(bp);
		print("[skipping %udbytes]\n", len);
		Bseek(bp, len, 1);
	}

	if (flags & FLfiledir)
		fileloc(bp);
	if (flags & FLdesc)
		print("description: %s\n", lenstr(bp));
	if (flags & FLrelative)
		print("relative-path: %s\n", lenstr(bp));
	if (flags & FLcwd)
		print("working-dir: %s\n", lenstr(bp));
	if (flags & FLargs)
		print("command-line: %s\n", lenstr(bp));
	if (flags & FLicon)
		print("icon-file: %s\n", lenstr(bp));
	Bterm(bp);
	return 0;
}

void
get(char *file)
{
	Biobuf *bp;
	char *rel, *lead, *tail, tmp[sizeof(magic)];
	int flags, base, len, voltype, baseoff, netoff, tailoff, shareoff;
	int type, spec, code;

	rel = "?";
	lead = "";
	tail = "";
	if ((bp = Bopen(file, OREAD)) == nil)
		sysfatal("%s - cannot open", file);

	gmem(bp, tmp, 20);	/* magic and GUID */
	if (memcmp(tmp, magic, sizeof(magic)) != 0)
		sysfatal("%s - bad magic", file);

	flags = g32(bp);
	g32(bp);		/* target attributes */
	gvtime(bp);		/* target creation time */
	gvtime(bp);		/* target modification time */
	gvtime(bp);		/* target access time */
	g32(bp);		/* target file size */
	g32(bp);		/* icon index in icon number */
	g32(bp);		/* "show window" code */
	g32(bp);		/* hotkey number */
	g32(bp);		/* unknown */
	g32(bp);		/* unknown */

	if (flags & FLidlist)
		Bseek(bp, g16(bp), 1);
	if (flags & FLfiledir){
		base = Boffset(bp);
		len = g32(bp);		/* reclen		 */
		g32(bp);		/* offset of 1st byte after this struct */
		voltype = g32(bp);	/* flags (d0 = local, d1 = network share) */
		g32(bp);		/* offset to local volume info */
		baseoff = g32(bp);	/* offset to base of path on local system */
		netoff = g32(bp);	/* offset of network volume info */
		tailoff = g32(bp);	/* offset of path tail */

		lead = strdup("");
		if (voltype & VOLlocal){
			Bseek(bp, base+baseoff, 0);
			lead = gstr(bp);
		}
		if (voltype & VOLnetwork){
			Bseek(bp, base+netoff, 0);
			g32(bp);		/* reclen */
			type = g32(bp);
			shareoff = g32(bp);
			spec = g32(bp);
			code = g32(bp);
			if (type == 2 && spec == 0 && code == 0x20000)
				print("proto=cifs\n");
			else if (type == 3 && (spec & 0xf0) == 0x20 &&
			    code == 0x30000)
				print("proto=ncp\n");
			else
				print("proto=unknown (0x%x,0x%x,0x%x)\n",
					type, spec, code);
			Bseek(bp, base+netoff+shareoff, 0);
			lead = gstr(bp);
		}
		Bseek(bp, base+tailoff, 0);
		tail = gstr(bp);
		Bseek(bp, base+len, 0);
	} else if (flags & FLdesc)
		print("desc=%q\n", lenstr(bp));
	if (flags & FLrelative)
		rel = lenstr(bp);
	if (flags & FLcwd)
		print("cwd=%q\n", tofwd(lenstr(bp)));
	if (flags & FLargs)
		print("args=%q\n", tofwd(lenstr(bp)));

	Bterm(bp);

	if (!*lead)
		print("path=%s\n", tofwd(rel));
	else
		print("path=%s%s%s\n", tofwd(lead), (*tail)? "/": "", tofwd(tail));
}

void
main(int argc, char *argv[])
{
	int i;

	if (argc < 2){
		fprint(2, "usage: %s linkfile [dst]\n", argv0);
		exits("usage");
	}

	fmtinstall('q', quotestrfmt);
	for (i = 1; i < argc; i++){
		dump(argv[i]);
//		get(argv[i]);
	}
	exits(0);
}
