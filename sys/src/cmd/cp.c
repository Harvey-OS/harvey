#include <u.h>
#include <libc.h>

#define	DEFB	(8*1024)

int	xflag;
void	copy(char *from, char *to, int todir);
void	copy1(int fdf, int fdt, char *from, char *to);

void
main(int argc, char *argv[])
{
	Dir dirb;
	int todir, i;

	ARGBEGIN {
	case 'x':
		xflag++;
		break;
	default:
		goto usage;
	} ARGEND

	todir=0;
	if(argc < 2)
		goto usage;
	if(dirstat(argv[argc-1], &dirb)==0 && (dirb.mode&CHDIR))
		todir=1;
	if(argc>2 && !todir){
		fprint(2, "cp: %s not a directory\n", argv[argc-1]);
		exits("bad usage");
	}
	for(i=0; i<argc-1; i++)
		copy(argv[i], argv[argc-1], todir);
	exits(0);

usage:
	fprint(2, "usage:\tcp [-x] fromfile tofile\n");
	fprint(2, "\tcp [-x] fromfile ... todir\n");
	exits("usage");
}

void
copy(char *from, char *to, int todir)
{
	Dir dirb, dirt;
	char name[256];
	int fdf, fdt;
	char *cp;

	if(todir){
		char *s, *elem;
		elem=s=from;
		while(*s++)
			if(s[-1]=='/')
				elem=s;
		sprint(name, "%s/%s", to, elem);
		to=name;
	}
	if(dirstat(from, &dirb)!=0){
		fprint(2,"cp: can't stat %s: %r\n", from);
		return;
	}
	if(xflag) {
		if(dirstat(to, &dirt)==0)
		if(dirb.qid.path==dirt.qid.path && dirb.qid.vers==dirt.qid.vers)
		if(dirb.dev==dirt.dev && dirb.type==dirt.type){
			fprint(2, "cp: %s and %s are the same file\n", from, to);
			return;
		}
		fdf=open(from, OREAD);
		if(fdf<0){
			fprint(2, "cp: can't open %s: %r\n", from);
			return;
		}
		if(dirb.mode&CHDIR) {
			fdt=create(to, OREAD, dirb.mode);
			if(fdt<0)
				fprint(2, "cp: can't mkdir %s: %r\n", to);
		} else {
			remove(to);
			fdt=create(to, OWRITE, dirb.mode);
			if(fdt<0){
				fprint(2, "cp: can't create %s: %r\n", to);
				close(fdf);
				return;
			}
			copy1(fdf, fdt, from, to);
		}
		close(fdf);
		close(fdt);
		cleanname(to);
		cp = strrchr(to, '/');
		if(cp)
			cp++;
		else
			cp = to;
		strncpy(dirb.name, cp, sizeof(dirb.name));
		if(dirwstat(to, &dirb)==-1){
			dirstat(to, &dirt);
			strncpy(dirb.uid, dirt.uid, sizeof(dirb.uid));
			if(dirwstat(to, &dirb)==0)
				fprint(2, "cp: can't wstat uid %s: %r\n", to);
			else{
				strncpy(dirb.gid, dirt.gid, sizeof(dirb.gid));
				if(dirwstat(to, &dirb)==0)
					fprint(2, "cp: can't wstat uid or gid %s: %r\n", to);
				else
					fprint(2, "cp: can't wstat %s: %r\n", to);
			}
		}
	} else {
		if(dirb.mode&CHDIR){
			fprint(2, "cp: %s is a directory\n", from);
			return;
		}
		dirb.mode &= 0777;
		if(dirstat(to, &dirt)==0)
		if(dirb.qid.path==dirt.qid.path && dirb.qid.vers==dirt.qid.vers)
		if(dirb.dev==dirt.dev && dirb.type==dirt.type){
			fprint(2, "cp: %s and %s are the same file\n", from, to);
			return;
		}
		fdf=open(from, OREAD);
		if(fdf<0){
			fprint(2, "cp: can't open %s: %r\n", from);
			return;
		}
		fdt=create(to, OWRITE, dirb.mode);
		if(fdt<0){
			fprint(2, "cp: can't create %s: %r\n", to);
			close(fdf);
			return;
		}
		copy1(fdf, fdt, from, to);
		close(fdf);
		close(fdt);
	}
}

void
copy1(int fdf, int fdt, char *from, char *to)
{
	char *buf;
	long n, n1, rcount;

	buf = malloc(DEFB);
	/* clear any residual error */
	memset(buf, 0, ERRLEN);
	errstr(buf);
	for(rcount=0;; rcount++) {
		n = read(fdf, buf, DEFB);
		if(n <= 0)
			break;
		n1 = write(fdt, buf, n);
		if(n1 != n) {
			fprint(2, "cp: error writing %s: %r\n", to);
			break;
		}
	}
	if(n < 0) 
		fprint(2, "cp: error reading %s: %r\n", from);
	free(buf);
}
