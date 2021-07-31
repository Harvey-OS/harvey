#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
/*
 * Bug: doesn't check status from malloc or realloc.
 */
PICFILE *picputprop(PICFILE *f, char *name, char *val){
	register char **ap;
	int i, nlen=strlen(name), vlen=strlen(val), alen;
	for(i=0,ap=f->argv;i!=f->argc;i++,ap++)
		if(strncmp(*ap, name, nlen)==0 && (*ap)[nlen]=='='){
			alen=strlen(*ap);
			*ap=realloc(*ap, alen+vlen+2);
			(*ap)[alen]='\n';
			strcpy(*ap+alen+1, val);
			return f;
		}
	f->argv=(char **)realloc((char *)f->argv, (f->argc+2)*sizeof(char *));
	ap=&f->argv[f->argc];
	*ap=malloc(nlen+vlen+2);
	strcpy(*ap, name);
	(*ap)[nlen]='=';
	strcpy(*ap+nlen+1, val);
	f->argv[++f->argc]=0;
	return f;
}
