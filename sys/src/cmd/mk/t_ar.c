#include	"mk.h"
#include	<ar.h>

static void atimes(char *);

long
atimeof(int force, char *name, char *ar)
{
	Symtab *sym;
	long t;

	t = mtime(ar);
	if(sym = symlook(ar, S_AGG, 0)){
		if(force || (t > (unsigned long)sym->value)){
			atimes(ar);
			sym->value = (char *)t;
		}
	} else {
		atimes(ar);
		/* mark the aggegate as having been done */
		symlook(strdup(ar), S_AGG, "")->value = (char *)t;
	}
	sym = symlook(name, S_TIME, 0);
	if (sym)
		return(long) sym->value;		/* uggh */
	else
		return 0;
}

void
atouch(char *name, char *ar, char *mem)
{
	int fd;
	struct ar_hdr hdr;
	char *s;
	long t;

	if((fd = open(ar, 2)) < 0){
		if((fd = create(ar, OWRITE, 0666)) < 0){
			perror(ar);
			Exit();
		}
		write(fd, ARMAG, (COUNT) SARMAG);
	}
	if(symlook(name, S_TIME, (char *)0)){
		/* hoon off and change it in situ */
		LSEEK(fd, (long)SARMAG, 0);
		while(read(fd, (char *)&hdr, (COUNT)sizeof(hdr)) == sizeof(hdr)){
			for(s = &hdr.name[sizeof(hdr.name)]; *--s == ' ';);
			s[1] = 0;
			if(strcmp(mem, hdr.name) == 0){
				t = sizeof(hdr.name)-sizeof(hdr);
				LSEEK(fd, t, 1);
				fprint(fd, "%-12ld", time((long *)0));
				break;
			}
			t = atol(hdr.size);
			if(t&01) t++;
			LSEEK(fd, t, 1);
		}
	} else {
		LSEEK(fd, 0L, 2);
		fprint(fd, "%-16s%-12ld%-6d%-6d%-8lo%-10ld%2s", mem, time((long *)0),
			getuid(), getgid(), 0100666L, 0L, ARFMAG);
	}
	close(fd);
}


void
adelete(char *name, char *ar, char *mem)
{
	USED(name, ar, mem);
	fprint(2, "hoon off; mk doesn't know how to delete archive members yet\n");
}

static void
atimes(char *ar)
{
	struct ar_hdr hdr;
	long t;
	int fd;
	char buf[BIGBLOCK];
	char *s;

	if((fd = open(ar, 0)) < 0)
		return;
	if(read(fd, buf, (COUNT)SARMAG) != SARMAG){
		close(fd);
		return;
	}
	while(read(fd, (char *)&hdr, (COUNT)sizeof(hdr)) == sizeof(hdr)){
		for(s = &hdr.name[sizeof(hdr.name)]; *--s == ' ';);
#ifdef	SYSV
		if(*s == '/')	/* Damn you Sytem V */
			s--;
#endif
		t = atol(hdr.date);
		if(t == 0)	/* as it sometimes happens; thanks ken */
			t = 1;
		/* Blows away the date field */
		s[1] = 0;
		sprint(buf, "%s(%s)", ar, hdr.name);
		symlook(strdup(buf), S_TIME, (char *)t)->value = (char *)t;
		t = atol(hdr.size);
		if(t&01) t++;
		LSEEK(fd, t, 1);
	}
	close(fd);
}
