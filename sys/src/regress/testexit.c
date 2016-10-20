#include <u.h>
#include <libc.h>

int _finishing = 0;
int _sessleader = 0;

static char exitstatus[ERRMAX];

/* emulate: return p+sprintf(p, "%uld", v) */
#define IDIGIT 15
char *
_ultoa(char *p, uint32_t v)
{
	char s[IDIGIT];
	int n, i;

	s[IDIGIT-1] = 0;
	for(i = IDIGIT-2; i; i--){
		n = v % 10;
		s[i] = n + '0';
		v = v / 10;
		if(v == 0)
			break;
	}
	strcpy(p, s+i);
	return p + (IDIGIT-1-i);
}

void
_finish(int status, char *term)
{
	char *cp;

	if(_finishing)
		_exits(exitstatus);
	_finishing = 1;
	if(status){
		cp = _ultoa(exitstatus, status & 0xFF);
		*cp = 0;
	}else if(term){
		strncpy(exitstatus, term, ERRMAX);
		exitstatus[ERRMAX-1] = '\0';
	}
//	if(_sessleader)
//		kill(0, SIGTERM);
	_exits(exitstatus);
}

void
_exit(int status)
{
	_finish(status, 0);
}

int main(int argc, char *argv[])
{
	int v = 5;
	if (argc > 1)
		v = strtoul(argv[1], 0, 0);
	_exit(v);
	return 0;
}
