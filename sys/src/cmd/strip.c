#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>

void
error(char* fmt, ...)
{
	va_list arg;
	char *e, s[256];

	va_start(arg, fmt);
	e = seprint(s, s+sizeof(s), "%s: ", argv0);
	e = vseprint(e, s+sizeof(s), fmt, arg);
	e = seprint(e, s+sizeof(s), "\n");
	va_end(arg);

	write(2, s, e-s);
}

static void
usage(void)
{
	error("usage: %s -o ofile file\n\t%s file ...\n", argv0, argv0);
	exits("usage");
}

static int
strip(char* file, char* out)
{
	Dir *dir;
	int fd, i;
	Fhdr fhdr;
	Exec *exec;
	ulong mode;
	void *data;
	vlong length;

	if((fd = open(file, OREAD)) < 0){
		error("%s: open: %r", file);
		return 1;
	}

	if(!crackhdr(fd, &fhdr)){
		error("%s: %r", file);
		close(fd);
		return 1;
	}
	for(i = MIN_MAGIC; i <= MAX_MAGIC; i++){
		if(fhdr.magic == _MAGIC(0, i) || fhdr.magic == _MAGIC(HDR_MAGIC, i))
			break;
	}
	if(i > MAX_MAGIC){
		error("%s: not a recognizeable binary", file);
		close(fd);
		return 1;
	}

	if((dir = dirfstat(fd)) == nil){
		error("%s: stat: %r", file);
		close(fd);
		return 1;
	}

	length = fhdr.datoff+fhdr.datsz;
	if(length == dir->length){
		if(out == nil){	/* nothing to do */
			error("%s: already stripped", file);
			free(dir);
			close(fd);
			return 0;
		}
	}
	if(length > dir->length){
		error("%s: strange length", file);
		close(fd);
		free(dir);
		return 1;
	}

	mode = dir->mode;
	free(dir);

	if((data = malloc(length)) == nil){
		error("%s: malloc failure", file);
		close(fd);
		return 1;
	}
	seek(fd, 0LL, 0);
	if(read(fd, data, length) != length){
		error("%s: read: %r", file);
		close(fd);
		free(data);
		return 1;
	}
	close(fd);

	exec = data;
	exec->syms = 0;
	exec->spsz = 0;
	exec->pcsz = 0;

	if(out == nil){
		if(remove(file) < 0) {
			error("%s: remove: %r", file);
			free(data);
			return 1;
		}
		out = file;
	}
	if((fd = create(out, OWRITE, mode)) < 0){
		error("%s: create: %r", out);
		free(data);
		return 1;
	}
	if(write(fd, data, length) != length){
		error("%s: write: %r", out);
		close(fd);
		free(data);
		return 1;
	}
	close(fd);
	free(data);

	return 0;
}

void
main(int argc, char* argv[])
{
	int r;
	char *p;

	p = nil;

	ARGBEGIN{
	default:
		usage();
		break;
	case 'o':
		p = ARGF();
		if(p == nil)
			usage();
		break;
	}ARGEND;

	switch(argc){
	case 0:
		usage();
		return;
	case 1:
		if(p != nil){
			r = strip(*argv, p);
			break;
		}
		/*FALLTHROUGH*/
	default:
		r = 0;
		while(argc > 0){
			r |= strip(*argv, nil);
			argc--;
			argv++;
		}
		break;
	}

	if(r)
		exits("error");
	exits(0);
}
