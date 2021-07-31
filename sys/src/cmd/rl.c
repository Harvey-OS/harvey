/*
 * rl.c -- rl
 */
#include <u.h>
#include <libc.h>
#include <ar.h>
#include <a.out.h>
#include <bio.h>
#include "obj.h"

enum
{
	RLENTSIZE	 = 4 + NNAME + 4,
};

Sym	*_sym;

char	*errs,
	*_filename,
	*libname,
	_firstname[NNAME],
	_symname[] = "__.SYMDEF";
int	_nsym,			/* number of symbols refed in current file */
	verbose,
	vflag;			/* verbose for reading of files */

Biobuf	*_bin;
Biobuf	_bout;

void	readar(void),
	writerl(void),
	setoffsets(Sym *, int),
	fexec(char *, char *, ...);

void
main(int argc, char *argv[])
{
	int i, n;
	char magbuf[SARMAG];

	Binit(&_bout, 1, OWRITE);
	argv0 = argv[0];
	ARGBEGIN {
	case 'v':
		verbose = 1;
		break;
	} ARGEND
	_assure(CHUNK);
	for(i=0; i<argc; i++){
		libname = _filename = argv[i];
		_bin = Bopen(_filename, OREAD);
		if(_bin == 0){
			fprint(2, "%s: cannot open %s\n", argv0, _filename);
			continue;
		}
		n = Bread(_bin, magbuf, SARMAG);
		if(n == SARMAG && strncmp(magbuf, ARMAG, SARMAG) == 0){
			readar();
			writerl();
		}else{
			fprint(2, "%s: not an archive: %s\n", argv0, libname);
			continue;
		}
		Bclose(_bin);
	}
	exits(errs);
}

/*
 * read an archive file,
 * processing the symbols for each intermediate file in it.
 */
void
readar(void)
{
	int first;

	_symsize = 0;
	_off = 0;
	first = 1;
	while(_nextar()){
		if(!_objsyms(1, first, setoffsets)){
			fprint(2, "warning: inconsistent file %s in %s\n", _filename, libname);
			return;
		}
		first = 0;
	}
	return;
}

/*
 * set offsets for all elements in a Sym array to the current file
 */
void
setoffsets(Sym *s, int nsym)
{
	int i;

	for(i=_global; i<nsym; i++)
		s[i].value = _off - _symsize;
}

void
writerl(void)
{
	Biobuf b;
	long new, off;
	int fd, i, n;

	fd = create(_symname, 1, 0664);
	if(fd < 0){
		fprint(2, "%s: cannot create %s\n", argv0, _symname);
		return;
	}
	Binit(&b, fd, OWRITE);
	n = 0;
	for(i = 0; i<_nsym; i++)
		if(_sym[i].type == 'T'
		|| _sym[i].type == 'D')
			n++;
	new = n * RLENTSIZE + SAR_HDR;
	for(i=0; i<_nsym; i++) {
		if(_sym[i].type != 'T'
		&& _sym[i].type != 'D')
			continue;
		off = _sym[i].value + new;
		if(verbose)
			print("%10s %d %ld\n",
				_sym[i].name,
				_sym[i].type == 'D',
				off);
		Bputc(&b, off);
		Bputc(&b, off>>8);
		Bputc(&b, off>>16);
		Bputc(&b, off>>24);
		n = Bwrite(&b, _sym[i].name, NNAME);
		if(n != NNAME)
			fprint(2, "%s: short write to %s\n", argv0, _symname);
		if(_sym[i].type == 'T')
			Bputc(&b, 0);
		else
			Bputc(&b, 1);
		/* padding */
		Bputc(&b, 0); Bputc(&b, 0); Bputc(&b, 0);
	}
	Bclose(&b);
	close(fd);
	if(strcmp(_firstname, _symname) != 0)
		if(_symsize == 0)
			fexec("/bin/ar", "ar", "rb", _firstname, libname, _symname, 0);
		else{
			fexec("/bin/ar", "ar", "mb", _firstname, libname, _symname, 0);
			fexec("/bin/ar", "ar", "r", libname, _symname, 0);
		}
	else
		fexec("/bin/ar", "ar", "r", libname, _symname, 0);
	remove(_symname);
}

void
fexec(char *av0, char *av, ...)
{
	int pid;
	Waitmsg w;

	switch(pid = fork()){
	case -1:
		fprint(2, "%s: can't fork\n", argv0);
		exits("fork");
	case 0:
		exec(av0, &av);
		fprint(2, "%s: can't exec\n", argv0);
		exits("exec");
	}
	while(wait(&w) != -1 && atoi(w.pid) != pid)
		;
}
