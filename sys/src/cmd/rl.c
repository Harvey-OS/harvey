/*
 * rl.c -- rl
 */
#include <u.h>
#include <libc.h>
#include <ar.h>
#include <bio.h>
#include <mach.h>

enum
{
	RLENTSIZE	 = 4 + NNAME + 4,
};

char	symname[]="__.SYMDEF";	/* table of contents file name */
char	*libname;		/* current archive */
char	firstname[NNAME];	/* first file in archive */
char	*errs;			/* exit status */
char	verbose;
int	nextern;		/* number of 'T' & 'D' symbols */
int	symsize;		/* size of __.SYMDEF archive member */

int	readrl(Biobuf *bp);
void	writerl(void),
	setoffsets(Sym *, int),
	fexec(char *, char *, ...);

void
main(int argc, char *argv[])
{
	int i;
	Biobuf	*bin;

	argv0 = argv[0];
	ARGBEGIN {
	case 'v':
		verbose = 1;
		break;
	} ARGEND
	for(i=0; i<argc; i++){
		libname = argv[i];
		bin = Bopen(libname, OREAD);
		if(bin == 0){
			fprint(2, "%s: cannot open %s\n", argv0, libname);
			errs = "errors";
			continue;
		}
		if (isar(bin)) {
			if (readrl(bin) < 0)
				errs = "errors";
			else
				writerl();
		}else{
			fprint(2, "%s: not an archive: %s\n", argv0, libname);
			errs = "errors";
		}
		Bclose(bin);
	}
	exits(errs);
}

/*
 * read an archive file,
 * processing the symbols for each intermediate file in it.
 */
int
readrl(Biobuf *bp)
{
	int i, j, offset, size, obj, lastobj;
	Sym *s;
	char membername[NNAME];

	offset = BOFFSET(bp);
	symsize = 0;
	lastobj = -1;
	objreset();
	j = 0;
	for (i = 0;;i++) {
		size = nextar(bp, offset, membername);
		if (size < 0) {
			fprint(2, "%s: phase error on ar header %ld\n", argv0, offset);
			return -1;
		}
		if (size == 0)
			return 1;
		if (i == 0)		/* first time through */
			strcpy(firstname, membername);
		if (strcmp(membername, symname) == 0) {
			symsize = size;		/* skip symbol table */
			offset += size;
			continue;
		}
		obj = objtype(bp);
		if (lastobj < 0)	/* force match first time & after err */
			lastobj = obj;
		if (obj < 0 || obj != lastobj) {
			fprint(2, "%s: inconsistent file %s in %s\n", argv0,
					membername, libname);
			return -1;
		}
		lastobj = obj;
		if (!readar(bp, obj, offset+size)) {
			fprint(2, "%s: invalid symbol reference in file %s\n", membername);
			return -1;
		}
		while (s = objsym(j)) {
			if (s->type == 'T' || s->type == 'D') {
				s->value = offset-symsize;
				nextern++;
			}
			j++;
		}
		offset += size;
	}
	return 1;
}

void
writerl(void)
{
	Biobuf b;
	Sym *s;
	long new, off;
	int fd, i;

	fd = create(symname, 1, 0664);
	if(fd < 0){
		fprint(2, "%s: cannot create %s\n", argv0, symname);
		return;
	}
	Binit(&b, fd, OWRITE);
	new = nextern * RLENTSIZE + SAR_HDR;
	for(i=0; s = objsym(i); i++) {
		if(s->type != 'T' && s->type != 'D')
			continue;
		off = s->value + new;
		if(verbose)
			print("%10s %d %ld\n", s->name, s->type == 'D', off);
		Bputc(&b, off);
		Bputc(&b, off>>8);
		Bputc(&b, off>>16);
		Bputc(&b, off>>24);
		if (Bwrite(&b, s->name, NNAME) != NNAME) {
			fprint(2, "%s: short write to %s\n", argv0, symname);
			errs = "errors";
		}
		if(s->type == 'T')
			Bputc(&b, 0);
		else
			Bputc(&b, 1);
		/* padding */
		Bputc(&b, 0); Bputc(&b, 0); Bputc(&b, 0);
	}
	Bclose(&b);
	close(fd);
	if(strcmp(firstname, symname) != 0)
		if(symsize == 0)
			fexec("/bin/ar", "ar", "rb", firstname, libname, symname, 0);
		else{
			fexec("/bin/ar", "ar", "mb", firstname, libname, symname, 0);
			fexec("/bin/ar", "ar", "r", libname, symname, 0);
		}
	else
		fexec("/bin/ar", "ar", "r", libname, symname, 0);
	remove(symname);
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
