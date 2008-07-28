#include	"mk.h"
#include	<ar.h>

static void atimes(char *);
static char *split(char*, char**);

long
atimeof(int force, char *name)
{
	Symtab *sym;
	long t;
	char *archive, *member, buf[512];

	archive = split(name, &member);
	if(archive == 0)
		Exit();

	t = mtime(archive);
	sym = symlook(archive, S_AGG, 0);
	if(sym){
		if(force || t > sym->u.value){
			atimes(archive);
			sym->u.value = t;
		}
	}
	else{
		atimes(archive);
		/* mark the aggegate as having been done */
		symlook(strdup(archive), S_AGG, "")->u.value = t;
	}
		/* truncate long member name to sizeof of name field in archive header */
	snprint(buf, sizeof(buf), "%s(%.*s)", archive, utfnlen(member, SARNAME), member);
	sym = symlook(buf, S_TIME, 0);
	if (sym)
		return sym->u.value;
	return 0;
}

void
atouch(char *name)
{
	char *archive, *member;
	int fd, i;
	struct ar_hdr h;
	long t;

	archive = split(name, &member);
	if(archive == 0)
		Exit();

	fd = open(archive, ORDWR);
	if(fd < 0){
		fd = create(archive, OWRITE, 0666);
		if(fd < 0){
			perror(archive);
			Exit();
		}
		write(fd, ARMAG, SARMAG);
	}
	if(symlook(name, S_TIME, 0)){
		/* hoon off and change it in situ */
		LSEEK(fd, SARMAG, 0);
		while(read(fd, (char *)&h, sizeof(h)) == sizeof(h)){
			for(i = SARNAME-1; i > 0 && h.name[i] == ' '; i--)
					;
			h.name[i+1]=0;
			if(strcmp(member, h.name) == 0){
				t = SARNAME-sizeof(h);	/* ughgghh */
				LSEEK(fd, t, 1);
				fprint(fd, "%-12ld", time(0));
				break;
			}
			t = atol(h.size);
			if(t&01) t++;
			LSEEK(fd, t, 1);
		}
	}
	close(fd);
}

static void
atimes(char *ar)
{
	struct ar_hdr h;
	long at, t;
	int fd, i;
	char buf[BIGBLOCK];
	Dir *d;
	
	fd = open(ar, OREAD);
	if(fd < 0)
		return;

	if(read(fd, buf, SARMAG) != SARMAG){
		close(fd);
		return;
	}
	if((d = dirfstat(fd)) == nil){
		close(fd);
		return;
	}
	at = d->mtime;
	free(d);
	while(read(fd, (char *)&h, sizeof(h)) == sizeof(h)){
		t = atol(h.date);
		if(t >= at)	/* new things in old archives confuses mk */
			t = at-1;
		if(t <= 0)	/* as it sometimes happens; thanks ken */
			t = 1;
		for(i = sizeof(h.name)-1; i > 0 && h.name[i] == ' '; i--)
			;
		if(h.name[i] == '/')		/* system V bug */
			i--;
		h.name[i+1]=0;		/* can stomp on date field */
		snprint(buf, sizeof buf, "%s(%s)", ar, h.name);
		symlook(strdup(buf), S_TIME, (void*)t)->u.value = t;
		t = atol(h.size);
		if(t&01) t++;
		LSEEK(fd, t, 1);
	}
	close(fd);
}

static int
type(char *file)
{
	int fd;
	char buf[SARMAG];

	fd = open(file, OREAD);
	if(fd < 0){
		if(symlook(file, S_BITCH, 0) == 0){
			Bprint(&bout, "%s doesn't exist: assuming it will be an archive\n", file);
			symlook(file, S_BITCH, (void *)file);
		}
		return 1;
	}
	if(read(fd, buf, SARMAG) != SARMAG){
		close(fd);
		return 0;
	}
	close(fd);
	return !strncmp(ARMAG, buf, SARMAG);
}

static char*
split(char *name, char **member)
{
	char *p, *q;

	p = strdup(name);
	q = utfrune(p, '(');
	if(q){
		*q++ = 0;
		if(member)
			*member = q;
		q = utfrune(q, ')');
		if (q)
			*q = 0;
		if(type(p))
			return p;
		free(p);
		fprint(2, "mk: '%s' is not an archive\n", name);
	}
	return 0;
}
