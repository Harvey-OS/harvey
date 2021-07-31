#include <u.h>
#include <libc.h>
#include "../libString/String.h"

typedef struct Qfile Qfile;
struct Qfile
{
	Qfile	*next;
	char	*name;
	char	*tname;
} *files;

int	copy(Qfile*);

void
usage(void)
{
	fprint(2, "usage: qer [-f file] q-root description reply-to arg-list\n");
	exits("usage");
}

void
error(char *f, char *a)
{
	char err[ERRLEN+1];
	char buf[256];

	errstr(err);
	sprint(buf, f, a);
	fprint(2, "qer: %s: %s\n", buf, err);
	exits(buf);	
}

void
main(int argc, char**argv)
{
	Dir	dir;
	String	*f, *c;
	int	fd;
	char	file[1024];
	char	buf[1024];
	long	n;
	char	*cp;
	int	i;
	Qfile	*q, **l;

	l = &files;

	ARGBEGIN {
	case 'f':
		q = malloc(sizeof(Qfile));
		q->name = ARGF();
		q->next = *l;
		*l = q;
		break;
	default:
		usage();
	} ARGEND;

	if(argc < 3)
		usage();

	sprint(file, "%s/%s", argv[0], getuser());

	if(dirstat(file, &dir) < 0){
		if((fd = create(file, OREAD, CHDIR|0774)) < 0)
			error("can't create directory %s", file);
		close(fd);
	} else {
		if((dir.qid.path&CHDIR)==0)
			error("not a directory %s", file);
	}

	/*
	 *  create data file
	 */
	f = s_copy(file);
	s_append(f, "/D.XXXXXX");
	mktemp(s_to_c(f));

	/*
	 *  copy over associated files
	 */
	if(files){
		cp = utfrrune(s_to_c(f), '/');
		cp++;
		*cp = 'F';
		for(q = files; q; q = q->next){
			q->tname = strdup(s_to_c(f));
			if(copy(q) < 0)
				error("copying %s to queue", q->name);
			(*cp)++;
		}
		*cp = 'D';
	}

	/*
	 *  copy in the data file
	 */
	fd = create(s_to_c(f), OWRITE, 0660);
	if(fd < 0)
		error("creating data file %s", s_to_c(f));
	while((n = read(0, buf, sizeof(buf))) > 0)
		if(write(fd, buf, n) != n)
			error("writing data file %s", s_to_c(f));
/*	if(n < 0)
		error("reading input"); */
	close(fd);

	/*
	 *  create control file
	 */
	cp = utfrrune(s_to_c(f), '/');
	cp++;
	*cp = 'C';
	fd = create(s_to_c(f), OWRITE, CHEXCL|0660);
	if(fd < 0)
		error("creating control file %s", s_to_c(f));
	c = s_new();
	for(i = 1; i < argc; i++){
		s_append(c, argv[i]);
		s_append(c, " ");
	}
	for(q = files; q; q = q->next){
		s_append(c, q->tname);
		s_append(c, " ");
	}
	s_append(c, "\n");
	if(write(fd, s_to_c(c), strlen(s_to_c(c))) < 0)
		error("writing control file %s", s_to_c(f));
	close(fd);
	exits(0);
}

int
copy(Qfile *q)
{
	int from, to, n;
	char buf[4096];

	from = open(q->name, OREAD);
	if(from < 0)
		return -1;
	to = create(q->tname, OWRITE, 0660);
	if(to < 0){
		close(from);
		return -1;
	}
	for(;;){
		n = read(from, buf, sizeof(buf));
		if(n <= 0)
			break;
		n = write(to, buf, n);
		if(n < 0)
			break;
	}
	close(to);
	close(from);
	return n;
}
