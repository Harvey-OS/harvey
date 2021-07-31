#include <stdlib.h>
#include <sys/utsname.h>
#include <stdio.h>

#define	ARGBEGIN	for((argv0=*argv),argv++,argc--;\
			    argv[0] && argv[0][0]=='-' && argv[0][1];\
			    argc--, argv++) {\
				char *_args, *_argt, _argc;\
				_args = &argv[0][1];\
				if(_args[0]=='-' && _args[1]==0){\
					argc--; argv++; break;\
				}\
				while(*_args) switch(_argc=*_args++)
#define	ARGEND		}
#define	ARGF()		(_argt=_args, _args="",\
				(*_argt? _argt: argv[1]? (argc--, *++argv): 0))
#define	ARGC()		_argc
char *argv0;

main(int argc, char **argv)
{
	struct utsname u;

	uname(&u);
	if(argc == 1){
		printf("%s\n", u.sysname);
		exit(0);
	}
	ARGBEGIN {
	case 'a':
		printf("%s %s %s %s %s\n", u.sysname, u.nodename,
			u.release, u.version, u.machine);
		break;
	case 'm':
		printf("%s\n", u.machine);
		break;
	case 'n':
		printf("%s\n", u.nodename);
		break;
	case 'r':
		printf("%s\n", u.release);
		break;
	case 's':
		printf("%s\n", u.sysname);
		break;
	case 'v':
		printf("%s\n", u.version);
		break;
	} ARGEND
	exit(0);
}
