#include <u.h>
#include <libc.h>

#define U(x) (x<<6)
#define G(x) (x<<3)
#define O(x) (x)
#define A(x) (U(x)|G(x)|O(x))

#define CHRWE (CHREAD|CHWRITE|CHEXEC)

int parsemode(char *, ulong *, ulong *);

void
main(int argc, char *argv[])
{
	int i;
	Dir dir;
	ulong mode, mask;
	char *p;
	char err[ERRLEN];

	if(argc < 2){
		fprint(2, "usage: chmod 0777 file ... or chmod [who]op[rwxal] file ...\n");
		exits("usage");
	}
	mode = strtol(argv[1], &p, 8);
	if(*p == 0)
		mask = A(CHRWE);
	else if(!parsemode(argv[1], &mask, &mode)){
		fprint(2, "chmod: bad mode: %s\n", argv[1]);
		exits("mode");
	}
	for(i=2; i<argc; i++){
		if(dirstat(argv[i], &dir)==-1){
			errstr(err);
			fprint(2, "chmod: can't stat %s: %s\n", argv[i], err);
			continue;
		}
		dir.mode = (dir.mode & ~mask) | (mode & mask);
		if(dirwstat(argv[i], &dir)==-1){
			errstr(err);
			fprint(2, "chmod: can't wstat %s: %s\n", argv[i], err);
			continue;
		}
	}
	exits(0);
}

int
parsemode(char *spec, ulong *pmask, ulong *pmode)
{
	ulong mode, mask;
	int done, op;
	char *s;

	s = spec;
	mask = CHAPPEND | CHEXCL;
	for(done=0; !done; ){
		switch(*s){
		case 'u':
			mask |= U(CHRWE); break;
		case 'g':
			mask |= G(CHRWE); break;
		case 'o':
			mask |= O(CHRWE); break;
		case 'a':
			mask |= A(CHRWE); break;
		case 0:
			return 0;
		default:
			done = 1;
		}
		if(!done)
			s++;
	}
	if(s == spec)
		mask |= A(CHRWE);
	op = *s++;
	if(op != '+' && op != '-' && op != '=')
		return 0;
	mode = 0;
	for(; *s ; s++){
		switch(*s){
		case 'r':
			mode |= A(CHREAD); break;
		case 'w':
			mode |= A(CHWRITE); break;
		case 'x':
			mode |= A(CHEXEC); break;
		case 'a':
			mode |= CHAPPEND; break;
		case 'l':
			mode |= CHEXCL; break;
		default:
			return 0;
		}
	}
	if(*s != 0)
		return 0;
	if(op == '+' || op == '-')
		mask &= mode;
	if(op == '-')
		mode = ~mode;
	*pmask = mask;
	*pmode = mode;
	return 1;
}
