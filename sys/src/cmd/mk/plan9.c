#include	"mk.h"

char 	*shell =	"/bin/rc";
char 	*shellname =	"rc";

static	Word	*encodenulls(char*, int);

void
readenv(void)
{
	char *p;
	int envf, f;
	Dir *e;
	char nam[1024];
	int i, n, len;
	Word *w;

	rfork(RFENVG);	/*  use copy of the current environment variables */

	envf = open("/env", OREAD);
	if(envf < 0)
		return;
	while((n = dirread(envf, &e)) > 0){
		for(i = 0; i < n; i++){
			len = e[i].length;
				/* don't import funny names, NULL values,
				 * or internal mk variables
				 */
			if(len <= 0 || *shname(e[i].name) != '\0')
				continue;
			if (symlook(e[i].name, S_INTERNAL, 0))
				continue;
			sprint(nam, "/env/%s", e[i].name);
			f = open(nam, OREAD);
			if(f < 0)
				continue;
			p = Malloc(len+1);
			if(read(f, p, len) != len){
				perror(nam);
				close(f);
				continue;
			}
			close(f);
			if (p[len-1] == 0)
				len--;
			else
				p[len] = 0;
			w = encodenulls(p, len);
			free(p);
			p = strdup(e[i].name);
			setvar(p, (void *) w);
			symlook(p, S_EXPORTED, (void*)"")->value = (void*)"";
		}
		free(e);
	}
	close(envf);
}

/* break string of values into words at 01's or nulls*/
static Word *
encodenulls(char *s, int n)
{
	Word *w, *head;
	char *cp;

	head = w = 0;
	while (n-- > 0) {
		for (cp = s; *cp && *cp != '\0'; cp++)
				n--;
		*cp = 0;
		if (w) {
			w->next = newword(s);
			w = w->next;
		} else
			head = w = newword(s);
		s = cp+1;
	}
	if (!head)
		head = newword("");
	return head;
}

/* as well as 01's, change blanks to nulls, so that rc will
 * treat the words as separate arguments
 */
void
exportenv(Envy *e)
{
	int f, n, hasvalue, first;
	Word *w;
	Symtab *sy;
	char nam[256];

	for(;e->name; e++){
		sy = symlook(e->name, S_VAR, 0);
		if (e->values == 0 || e->values->s == 0 || e->values->s[0] == 0)
			hasvalue = 0;
		else
			hasvalue = 1;
		if(sy == 0 && !hasvalue)	/* non-existant null symbol */
			continue;
		sprint(nam, "/env/%s", e->name);
		if (sy != 0 && !hasvalue) {	/* Remove from environment */
				/* we could remove it from the symbol table
				 * too, but we're in the child copy, and it
				 * would still remain in the parent's table.
				 */
			remove(nam);
			delword(e->values);
			e->values = 0;		/* memory leak */
			continue;
		}
	
		f = create(nam, OWRITE, 0666L);
		if(f < 0) {
			fprint(2, "can't create %s, f=%d\n", nam, f);
			perror(nam);
			continue;
		}
		first = 1;
		for (w = e->values; w; w = w->next) {
			n = strlen(w->s);
			if (n) {
				if(first)
					first = 0;
				else{
					if (write (f, "\0", 1) != 1)
						perror(nam);
				}
				if (write(f, w->s, n) != n)
					perror(nam);
			}
		}
		close(f);
	}
}

int
waitfor(char *msg)
{
	Waitmsg *w;
	int pid;

	if((w=wait()) == nil)
		return -1;
	strecpy(msg, msg+ERRMAX, w->msg);
	pid = w->pid;
	free(w);
	return pid;
}

void
expunge(int pid, char *msg)
{
	postnote(PNPROC, pid, msg);
}

int
execsh(char *args, char *cmd, Bufblock *buf, Envy *e)
{
	char *p;
	int tot, n, pid, in[2], out[2];

	if(buf && pipe(out) < 0){
		perror("pipe");
		Exit();
	}
	pid = rfork(RFPROC|RFFDG|RFENVG);
	if(pid < 0){
		perror("mk rfork");
		Exit();
	}
	if(pid == 0){
		if(buf)
			close(out[0]);
		if(pipe(in) < 0){
			perror("pipe");
			Exit();
		}
		pid = fork();
		if(pid < 0){
			perror("mk fork");
			Exit();
		}
		if(pid != 0){
			dup(in[0], 0);
			if(buf){
				dup(out[1], 1);
				close(out[1]);
			}
			close(in[0]);
			close(in[1]);
			if (e)
				exportenv(e);
			if(shflags)
				execl(shell, shellname, shflags, args, 0);
			else
				execl(shell, shellname, args, 0);
			perror(shell);
			_exits("exec");
		}
		close(out[1]);
		close(in[0]);
		p = cmd+strlen(cmd);
		while(cmd < p){
			n = write(in[1], cmd, p-cmd);
			if(n < 0)
				break;
			cmd += n;
		}
		close(in[1]);
		_exits(0);
	}
	if(buf){
		close(out[1]);
		tot = 0;
		for(;;){
			if (buf->current >= buf->end)
				growbuf(buf);
			n = read(out[0], buf->current, buf->end-buf->current);
			if(n <= 0)
				break;
			buf->current += n;
			tot += n;
		}
		if (tot && buf->current[-1] == '\n')
			buf->current--;
		close(out[0]);
	}
	return pid;
}

int
pipecmd(char *cmd, Envy *e, int *fd)
{
	int pid, pfd[2];

	if(DEBUG(D_EXEC))
		fprint(1, "pipecmd='%s'\n", cmd);/**/

	if(fd && pipe(pfd) < 0){
		perror("pipe");
		Exit();
	}
	pid = rfork(RFPROC|RFFDG|RFENVG);
	if(pid < 0){
		perror("mk fork");
		Exit();
	}
	if(pid == 0){
		if(fd){
			close(pfd[0]);
			dup(pfd[1], 1);
			close(pfd[1]);
		}
		if(e)
			exportenv(e);
		if(shflags)
			execl(shell, shellname, shflags, "-c", cmd, 0);
		else
			execl(shell, shellname, "-c", cmd, 0);
		perror(shell);
		_exits("exec");
	}
	if(fd){
		close(pfd[1]);
		*fd = pfd[0];
	}
	return pid;
}

void
Exit(void)
{
	while(waitpid() >= 0)
		;
	exits("error");
}

int
notifyf(void *a, char *msg)
{
	static int nnote;

	USED(a);
	if(++nnote > 100){	/* until andrew fixes his program */
		fprint(2, "mk: too many notes\n");
		notify(0);
		abort();
	}
	if(strcmp(msg, "interrupt")!=0 && strcmp(msg, "hangup")!=0)
		return 0;
	killchildren(msg);
	return -1;
}

void
catchnotes()
{
	atnotify(notifyf, 1);
}

char*
maketmp(void)
{
	static char temp[] = "/tmp/mkargXXXXXX";

	mktemp(temp);
	return temp;
}

int
chgtime(char *name)
{
	Dir sbuf;

	if(access(name, AEXIST) >= 0) {
		nulldir(&sbuf);
		sbuf.mtime = time((long *)0);
		return dirwstat(name, &sbuf);
	}
	return close(create(name, OWRITE, 0666));
}

void
rcopy(char **to, Resub *match, int n)
{
	int c;
	char *p;

	*to = match->sp;		/* stem0 matches complete target */
	for(to++, match++; --n > 0; to++, match++){
		if(match->sp && match->ep){
			p = match->ep;
			c = *p;
			*p = 0;
			*to = strdup(match->sp);
			*p = c;
		}
		else
			*to = 0;
	}
}

ulong
mkmtime(char *name)
{
	Dir *buf;
	ulong t;

	buf = dirstat(name);
	if(buf == nil)
		return 0;
	t = buf->mtime;
	free(buf);
	return t;
}
