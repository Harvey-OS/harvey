#include <u.h>
#include <libc.h>

void
erase(char *part)
{
	char file[256];
	int fd;

	snprint(file, sizeof file, "%sctl", part);
	fd = open(file, ORDWR);
	if(fd < 0)
		return;
	fprint(fd, "erase");
	close(fd);
}

char*
readfile(char *file)
{
	char buf[512];
	int n, fd;
	uchar *p;

	fd = open(file, OREAD);
	if(fd < 0)
		sysfatal("opening %s: %r", file);
	n = read(fd, buf, sizeof(buf)-1);
	close(fd);
	if(n < 0)
		return "";
	buf[n] = 0;
	for(p = (uchar*)buf; *p; p++)
		if(*p == 0xff){
			*p = 0;
			break;
		}
	return strdup(buf);
}

void
writefile(char *file, char *data)
{
	int fd;

	fd = open(file, OWRITE);
	if(fd < 0)
		fd = create(file, OWRITE, 0664);
	if(fd < 0)
		return;
	write(fd, data, strlen(data));
	close(fd);
}

void
main(int argc, char **argv)
{
	int from = 0;
	char *params;
	char *file = "/tmp/tmpparams";
	char *part;

	ARGBEGIN {
	case 'f':
		from++;
		break;
	} ARGEND;

	if(argc)
		part = argv[0];
	else
		part = "/dev/flash/user";

	if(from){
		params = readfile(part);
		writefile(file, params);
	} else {
		params = readfile(file);
		erase(part);
		writefile(part, params);
		free(params);
	}
}
