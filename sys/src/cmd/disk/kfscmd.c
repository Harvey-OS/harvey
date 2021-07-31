#include <u.h>
#include <libc.h>

void main(int argc, char *argv[]){
	char *name, buf[4*1024], err[ERRLEN];
	int fd, n, i, errs;

	name = 0;
	ARGBEGIN{
	case 'n':
		name = ARGF();
		if(strlen(name) >= NAMELEN){
			fprint(2, "kfscmd: server name too long\n");
			exits("bad server");
		}
		break;
	default:
		fprint(2, "usage: kfscmd [-n server] commands\n");
		exits("usage");
	}ARGEND

	if(name)
		sprint(buf, "/srv/kfs.%s.cmd", name);
	else
		strcpy(buf, "/srv/kfs.cmd");
	fd = open(buf, ORDWR);
	if(fd >= 0){
		errs = 0;
		for(i = 0; i < argc; i++){
			if(write(fd, argv[i], strlen(argv[i])) != strlen(argv[i])){
				fprint(2, "%s: error writing %s: %r", argv0, argv[i]);
				errs++;
				continue;
			}
			for(;;){
				n = read(fd, buf, sizeof buf - 1);
				if(n < 0){
					fprint(2, "%s: error executing %s: %r", argv0, argv[i]);
					errs++;
					break;
				}
				buf[n] = '\0';
				if(strcmp(buf, "done") == 0 || strcmp(buf, "success") == 0)
					break;
				if(strcmp(buf, "unknown command") == 0){
					errs++;
					print("kfscmd: command %s not recognized\n", argv[i]);
					break;
				}
				write(1, buf, n);
			}
		}
		exits(errs ? "errors" : 0);
	}
		
	fd = open("/srv/kfs", ORDWR);
	if(fd < 0){
		fprint(2, "%s: can't open /srv/kfs\n", argv0);
		exits("no kfs");
	}
	err[0] = '\0';
	errs = 0;
	for(i = 0; i < argc; i++){
		mount(fd, "/dev/null", MAFTER, argv[i], "");
		errstr(err);
		if(strcmp(err, "success") != 0){
			fprint(2, "%s: error executing %s: %s\n", argv0, argv[i], err);
			errs++;
		}
	}
	exits(errs ? "errors" : 0);
}
