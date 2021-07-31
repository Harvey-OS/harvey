#include <u.h>
#include <libc.h>

#define	MAXM	10
#define	DEFZ	(6*1024 - 8)

void	copy(char *from, char *to, int todir);
void	copy1(int fdf, int fdt, char *from, char *to);
long	zsize;

void
main(int argc, char *argv[])
{
	Dir dirb;
	int todir, i;
	char *s;

	zsize = 0;
	ARGBEGIN{
	case 'z':
		zsize = DEFZ;
		if(s = ARGF()){
			if(*s >= '0' && *s <= '9')
				zsize = atol(s);
		}
		break;
	}ARGEND
	if(argc<2){
		fprint(2, "usage:\tcp [-z[#]] fromfile tofile\n");
		fprint(2, "\tcp [-z[#]] fromfile ... todir\n");
		exits("usage");
	}
	todir=0;
	if(dirstat(argv[argc-1], &dirb)==0 && (dirb.mode&0x80000000L))
		todir=1;
	if(argc>2 && !todir){
		fprint(2, "cp: %s not a directory\n", argv[argc-1]);
		exits("bad usage");
	}
	for(i=0; i<argc-1; i++)
		copy(argv[i], argv[argc-1], todir);
	exits(0);
}

void
copy(char *from, char *to, int todir)
{
	Dir dirb, dirt;
	char name[256];
	int fdf, fdt;

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
		fprint(2, "cp: can't stat %s: ", from);
		perror("");
		return;
	}
	if(dirb.mode&0x80000000L){
		fprint(2, "cp: %s is a directory\n", from);
		return;
	}
	dirb.mode &= 0777;
	if(dirstat(to, &dirt)==0)
	if(dirb.qid.path==dirt.qid.path && dirb.qid.vers==dirt.qid.vers)
	if(dirb.dev==dirt.dev && dirb.type==dirt.type){
		fprint(2, "cp: %s and %s are same file\n", from, to);
		return;
	}
	fdf=open(from, OREAD);
	if(fdf<0){
		fprint(2, "cp: can't open %s:", from);
		perror("");
		return;
	}
	fdt=create(to, OWRITE, dirb.mode);
	if(fdt<0){
		fprint(2, "cp: can't create %s:", to);
		perror("");
		close(fdf);
		return;
	}
	copy1(fdf, fdt, from, to);
	close(fdf);
	close(fdt);
}

void
copy1(int fdf, int fdt, char *from, char *to)
{
	char *buf, *zbuf;
	long n, n1, bsize, rcount;
	int flag;

	if(zsize) {
		buf = malloc(zsize);
		zbuf = malloc(zsize);
		memset(zbuf, 0, zsize);
		flag = 0;
		for(;;) {
			n = read(fdf, buf, zsize);
			if(n != zsize) {
				if(n == 0)
					break;
				if(n < 0) {
					fprint(2, "cp: error reading %s:", from);
					perror("");
					break;
				}
				n1 = write(fdt, buf, n);
				if(n != n1) {
					fprint(2, "cp: error writing %s:", to);
					perror("");
					break;
				}
				n = read(fdf, buf, zsize);
				if(n != 0) {
					fprint(2, "cp: short read reading %s:", from);
					perror("");
					break;
				}
				flag = 0;
				break;
			}
			if(memcmp(buf, zbuf, zsize)) {
				n1 = write(fdt, buf, zsize);
				if(n1 != zsize) {
					fprint(2, "short write\n");
					exits("short write");
				}
				flag = 0;
				continue;
			}
			seek(fdt, zsize, 1);
			flag = 1;
		}
		if(flag) {
			seek(fdt, -1, 1);
			write(fdt, zbuf, 1);
		}
		return;
	}
	bsize = 8192;
	buf = malloc(bsize);
	for(rcount=0;; rcount++) {
		n = read(fdf, buf, bsize);
		if(n <= 0)
			break;
		n1 = write(fdt, buf, n);
		if(n1 != n) {
			fprint(2, "cp: error writing %s:", to);
			perror("");
			break;
		}
	}
	if(n < 0) {
		fprint(2, "cp: error reading %s:", from);
		perror("");
	}
}
