#include <u.h>
#include <libc.h>

#define U(x) (x<<6)
#define G(x) (x<<3)
#define O(x) (x)
#define A(x) (U(x)|G(x)|O(x))

#define DMRWE (DMREAD|DMWRITE|DMEXEC)

int parsemode(char *, ulong *, ulong *);

void
main(int argc, char *argv[])
{
	int i;
	Dir *dir, ndir;
	ulong mode, mask;
	char *p;

	if(argc < 3){
		fprint(2, "usage: chmod 0777 file ... or chmod [who]op[rwxalt] file ...\n");
		exits("usage");
	}
	mode = strtol(argv[1], &p, 8);
	if(*p == 0)
		mask = A(DMRWE);
	else if(!parsemode(argv[1], &mask, &mode)){
		fprint(2, "chmod: bad mode: %s\n", argv[1]);
		exits("mode");
	}
	nulldir(&ndir);
	for(i=2; i<argc; i++){
		dir = dirstat(argv[i]);
		if(dir == nil){
			fprint(2, "chmod: can't stat %s: %r\n", argv[i]);
			continue;
		}
		ndir.mode = (dir->mode & ~mask) | (mode & mask);
		free(dir);
		if(dirwstat(argv[i], &ndir)==-1){
			fprint(2, "chmod: can't wstat %s: %r\n", argv[i]);
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
	mask = DMAPPEND | DMEXCL | DMTMP;
	for(done=0; !done; ){
		switch(*s){
		case 'u':
			mask |= U(DMRWE); break;
		case 'g':
			mask |= G(DMRWE); break;
		case 'o':
			mask |= O(DMRWE); break;
		case 'a':
			mask |= A(DMRWE); break;
		case 0:
			return 0;
		default:
			done = 1;
		}
		if(!done)
			s++;
	}
	if(s == spec)
		mask |= A(DMRWE);
	op = *s++;
	if(op != '+' && op != '-' && op != '=')
		return 0;
	mode = 0;
	for(; *s ; s++){
		switch(*s){
		case 'r':
			mode |= A(DMREAD); break;
		case 'w':
			mode |= A(DMWRITE); break;
		case 'x':
			mode |= A(DMEXEC); break;
		case 'a':
			mode |= DMAPPEND; break;
		case 'l':
			mode |= DMEXCL; break;
		case 't':
			mode |= DMTMP; break;
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
