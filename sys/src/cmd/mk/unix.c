#include	"mk.h"
#include	<dirent.h>
#include	<sys/signal.h>

void
initenv(void)
{

	extern char **environ;
	char **p, *s;
	Word *w;

	for(p = environ; *p; p++){
		s = shname(*p);
		if(*s == '=') {
			*s = 0;
			w = stow(s+1);
		} else
			w = stow("");
		if (internalvar(*p, w))
			free(w);	/* dump only first word */
		else {
			s = strdup(*p);
			setvar(s, (char *)w);
			symlook(s, S_EXPORTED, "")->value = "";
		}
	}
}

void
exportenv(Envy *env, int n)
{
	extern char **environ;
	char **p;
	char buf[4096];

	environ = (char**)Malloc((n+1)*sizeof(char*));
	p = environ;
	while(n-- > 0 && env->name) {
		if(env->values)
			sprint(buf, "%s=%s", env->name,  wtos(env->values));
		else
			sprint(buf, "%s=", env->name);
		*p++ = strdup(buf);
		env++;
		
	}
	*p = 0;
}

void
dirtime(char *dir, char *path)
{
	DIR *dirp;
	struct dirent *dp;
	Dir d;
	char *t;
	char buf[4096];

	dirp = opendir(dir);
	if(dirp) {
		while((dp = readdir(dirp)) != 0){
			if(dirstat(dp->d_name, &d) < 0)
				continue;
			t = (char *)d.mtime;
			if (!t)			/* zero mode file */
				continue;
			sprint(buf, "%s%s", path, dp->d_name);
			if(symlook(buf, S_TIME, 0))
				continue;
			symlook(strdup(buf), S_TIME, t)->value = t;
		}
		closedir(dirp);
	}
}


int
waitfor(char *msg)
{
	int status;
	int pid;

	*msg = 0;
	pid = _wait(&status);
	if(pid > 0) {
		if(status&0x7f) {
			if(status&0x80)
				snprint(msg, ERRLEN, "signal %d, core dumped", status&0x7f);
			else
				snprint(msg, ERRLEN, "signal %d", status&0x7f);
		} else if(status&0xff00)
			snprint(msg, ERRLEN, "exit(%d)", (status>>8)&0xff);
	}
	return pid;
}

void
expunge(int pid, char *msg)
{
	if(strcmp(msg, "interrupt"))
		kill(pid, SIGINT);
	else
		kill(pid, SIGHUP);
}

char *
nextword(char *s, char **start, char **end)
{
	char *to;
	Rune r, q;
	int n;

	while (*s && SEP(*s))		/* skip leading white space */
		s++;
	to = *start = s;
	while (*s) {
		n = chartorune(&r, s);
		if (SEP(r))
			break;
		while(n--)
			*to++ = *s++;
		if(r == '\'' || r == '"') {
			q = r;
			while(*s) {
				n = chartorune(&r, s);
				if(r == '\\' && s[1] == q)
					n++;		/*pick up escaped quote*/
				while(n--)
					*to++ = *s++;
				if(r == q)
					break;
			}
		}
	}
	*end = to;
	return s;
}
