#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
char *picgetprop(PICFILE *f, char *name){
	int i, len=strlen(name);
	for(i=0;i!=f->argc;i++)
		if(strncmp(f->argv[i], name, len)==0 && f->argv[i][len]=='=')
			return f->argv[i]+len+1;
	return 0;
}
