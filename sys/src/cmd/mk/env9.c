#include	"mk.h"

void
initenv(void)
{
	char *ss, *sn;
	int envf, f;
	Dir e[20];
	char nam[NAMELEN+5];
	int i, n, len;
	Word *w;

	envf = open("/env", OREAD);
	if(envf < 0)
		return;
	while((n = dirread(envf, e, sizeof e)) > 0){
		n /= sizeof e[0];
		for(i = 0; i < n; i++){
			len = e[i].length;
			if(*shname(e[i].name) != '\0'	/* reject funny names */
			|| len <= 0)			/* and small variables */
				continue;
			sprint(nam, "/env/%s", e[i].name);
			f = open(nam, OREAD);
			if(f < 0)
				continue;
			ss = Malloc(len+1);
			if(read(f, ss, len) != len){
				perror(nam);
				close(f);
				continue;
			}
			close(f);
			if (ss[len-1] == 0)
				len--;
			else
				ss[len] = 0;
			w = encodenulls(ss, len);
			free(ss);
			if (internalvar(e[i].name, w))
				free(w);	/* dump only first word */
			else {
				sn = strdup(e[i].name);
				setvar(sn, (char *) w);
				symlook(sn, S_EXPORTED, "")->value = "";
			}
		}
	}
	close(envf);
}

void
exportenv(Envy *env)
{
	int hasvalue;
	Symtab *sy;
	char nam[NAMELEN+5];

	for (; env->name; env++) {
		sy = symlook(env->name, S_VAR, 0);
		if (env->values == 0 || env->values->s == 0
				|| env->values->s[0] == 0)
			hasvalue = 0;
		else
			hasvalue = 1;
		if(sy==0 && !hasvalue)	/* non-existant null symbol */
			continue;
		sprint(nam, "/env/%s", env->name);
		if (sy != 0 && !hasvalue) {	/* Remove from environment */
				/* we could remove it from the symbol table
				 * too, but we're in the child copy, and it
				 * would still remain in the parent's table.
				 */
			remove(nam);
			env->values = 0;	/* memory leak */
			continue;
		}
		export(nam, env->values);
	}
}

/* change any nulls in the first n characters of s to 01's */
Word *
encodenulls(char *s, int n)
{
	Word *w, *head;
	char *cp;

	head = w = 0;
	while (n-- > 0) {
		for (cp = s; *cp && *cp != '\01'; cp++)
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
export(char *name, Word *values)
{
	int f;
	int n;

	f = create(name, OWRITE, 0666L);
	if(f < 0) {
		fprint(2, "can't create %s, f=%d\n", name, f);
		perror(name);
		return;
	}
	while (values) {
		n = strlen(values->s);
		if (n) {
			if (write(f, values->s, n) != n)
				perror(name);
			if (write (f, "\0", 1) != 1)
				perror(name);
		}
		values = values->next;
	}
	close(f);
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
	kflag = 1;	/* to make sure waitup doesn't exit */
	jobs = 0;	/* make sure no more get scheduled */
	killchildren(msg);
	while(waitup(1, (int *)0) == 0)
		;
	Bprint(&stdout, "mk: %s\n", msg);
	Exit();
	return -1;
}

void
sigcatch(void)
{
	atnotify(notifyf, 1);
}
