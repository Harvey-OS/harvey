#include <u.h>
#include <libc.h>
#include "../libString/String.h"

void
usage(void)
{
	fprint(2, "usage: qer q-root description reply-to arg-list\n");
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
main(int ac, char**av)
{
	Dir	dir;
	String	*f, *c;
	int	fd;
	char	file[1024];
	char	buf[1024];
	long	n;
	char	*cp;
	int	i;

	if(ac < 5)
		usage();

	sprint(file, "%s/%s", av[1], getuser());

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
	for(i = 2; i < ac; i++){
		s_append(c, av[i]);
		s_append(c, " ");
	}
	s_append(c, "\n");
	if(write(fd, s_to_c(c), strlen(s_to_c(c))) < 0)
		error("writing control file %s", s_to_c(f));
	close(fd);
	exits(0);
}
