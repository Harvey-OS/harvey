#include <ctype.h>
#include "common.h"
#include "smtp.h"
#include "y.tab.h"

Biobuf	bout;

void
usage(void)
{
	fprint(2, "usage: to\n");
	exits("usage"); 
}
void
main(int argc, char **argv)
{
	int i, n, eof, pid, ac;
	Field *f, **l;
	Node *np;
	int pfd[2];
	char *uneaten;
	char buf[16*1024];
	char **av;
	Waitmsg w;
	int nav;

	/*
	 *  input the first 16k bytes.  The header had better fit.
	 */
	eof = 0;
	for(n = 0; n < sizeof(buf) - 1; n += i){
		i = read(0, buf+n, sizeof(buf)-1-n);
		if(i <= 0){
			eof = 1;
			break;
		}
	}
	buf[n] = 0;

	/*
	 *  skip any 'From ' line at the head
	 */
	i = 0;
	if(strncmp(buf, "From ", 5) == 0){
		for(; buf[i]; i++)
			if(buf[i] == '\n'){
				i++;
				break;
			}
	}

	/*
	 *  parse the header, turn all addresses into @ format
	 */
	yyinit(buf+i);
	yyparse();

	ac = 0;
	nav = 128;
	av = (char**)malloc(nav*sizeof(char*));
	av[ac++] = "/bin/mail";
	av[ac++] = "-r";
	ARGBEGIN {
	case 'x':
		av[ac++] = "-x";
		break;
	case '#':
		av[ac++] = "-#";
		break;
	} ARGEND;

	/*
	 *  make a list of destinations, take out bcc
	 */
	uneaten = buf+i;
	l = &firstfield;
	for(f = firstfield; f; f = f->next){
		switch(f->node->c){
		case TO:
		case BCC:
		case CC:
			break;
		default:
			for(np = f->node; np; np = np->next)
				uneaten = np->end;
			l = &f->next;
			uneaten++;		/* skip newline */
			continue;
		}
		for(np = f->node; np; np = np->next){
			uneaten = np->end;
			if(np->addr){
				if(ac == nav-2){
					nav *= 2;
					av = (char**)realloc(av, nav*sizeof(char*));
					if(av == 0){
						fprint(2, "%s: %r\n", argv0);
						exits("memory");
					}
				}
				av[ac++] = s_to_c(np->s);
			}
		}
		if(f->node->c == BCC)
			*l = f->next;
		else
			l = &f->next;
		uneaten++;		/* skip newline */
	}
	av[ac] = 0;

	if(pipe(pfd) < 0){
		fprint(2, "to: %r\n");
		exits("pipe");
	}

	switch(pid = fork()){
	case -1:
		fprint(2, "to: %r\n");
		exits("fork");
	case 0:
		dup(pfd[0], 0);
		close(pfd[0]);
		close(pfd[1]);
		exec("/bin/upas/sendmail", av);/**/
	}

	close(pfd[0]);
	Binit(&bout, pfd[1], OWRITE);

	/*
	 *  pipe headers to command
	 */
	for(f = firstfield; f; f = f->next){
		for(np = f->node; np; np = np->next){
			if(isprint(np->c&0xff))
				Bprint(&bout, "%c", np->c);
			if(np->s)
				Bprint(&bout, "%s", s_to_c(np->s));
			if(np->white)
				Bprint(&bout, "%s", s_to_c(np->white));
		}
		Bprint(&bout, "\n");
	}
	if (*uneaten != '\n')
		Bprint(&bout, "\n");

	/*
	 *  pipe rest of message to command
	 */
	if(n > uneaten-buf)
		Bwrite(&bout, uneaten, n-(uneaten-buf));

	if(eof == 0)
		while((n = read(0, buf, sizeof(buf))) > 0)
			Bwrite(&bout, buf, n);
	Bterm(&bout);
	close(pfd[1]);

	/*
	 */
	for(;;){
		n = wait(&w);
		if(n < 0)
			exits(0);
		if(n == pid)
			exits(w.msg);
	}
}
