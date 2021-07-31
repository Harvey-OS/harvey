#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
#define	NLINE	1024		/* maximum length of header line */
_PWRerror(PICFILE *f, char *buf){
	USED(f,buf);
	return 0;
}
static char *scanint(char *s, int *v){
	int n, sign=1;
	while(*s==' ' || *s=='\t') s++;
	if(*s=='-'){
		sign=-1;
		s++;
	}
	else if(*s=='+') s++;
	if(*s<'0' || '9'<*s) return 0;
	n=0;
	do{
		n=n*10+ *s++ -'0';
	}while('0'<=*s && *s<='9');
	*v=n*sign;
	return s;
}
PICFILE *picopen_r(char *file){
	char line[NLINE+1];
	char *lp;
	char *nchan, *window, *command;
	int x0, y0, x1, y1, i, n, first;
	PICFILE *p=(PICFILE *)malloc(sizeof(PICFILE));
	if(p==0){
		werrstr("Can't allocate PICFILE");
		return 0;
	}
	p->flags=PIC_NOCLOSE|PIC_INPUT;
	p->argc=-1;
	p->cmap=0;
	if(strcmp(file, "IN")==0)
		p->fd=dup(0, -1);
	else{
		p->fd=open(file, 0);
		if(p->fd<0){
		Error:
			if(!(p->flags&PIC_NOCLOSE)) close(p->fd);
			if(p->argc>=0){
				for(i=0;i!=p->argc;i++) free(p->argv[i]);
				free((char *)p->argv);
			}
			if(p->cmap) free(p->cmap);
			free((char *)p);
			return 0;
		}
		p->flags=PIC_INPUT;
	}
	p->argc=0;
	p->argv=(char **)malloc(sizeof(char *));
	p->argv[0]=0;
	first=1;
	for(;;){
		lp=line;
		do{
			if(lp==&line[NLINE]){
				werrstr("header line too long");
				goto Error;
			}
			if(read(p->fd, lp, 1)!=1){
				werrstr("Can't read header");
				goto Error;
			}
			if(first && lp==line+12*5-1 && _PICplan9header(p, line))
				goto AllRead;
		}while(*lp++!='\n');
		first=0;
		if(lp==line+1)
			break;
		lp[-1]='\0';
		lp=strchr(line, '=');
		if(lp==0){
			werrstr("Ill-formed header line");
			goto Error;
		}
		*lp++='\0';
		picputprop(p, line, lp);
	}
AllRead:
	if(strncmp(p->argv[0], "TYPE=", 5)!=0){
		werrstr("First header line not TYPE=");
		goto Error;
	}
	p->type=p->argv[0]+5;
	for(i=0;_PICconf[i].type;i++)
		if(strcmp(p->type, _PICconf[i].type)==0) break;
	if(!_PICconf[i].type){
		werrstr("Illegal TYPE");
		goto Error;
	}
	p->rd=_PICconf[i].rd;
	p->wr=_PWRerror;
	p->cl=_PICconf[i].cl;
	p->line=0;
	p->buf=0;
	p->chan=picgetprop(p, "CHAN");
	nchan=picgetprop(p, "NCHAN");
	if(_PICconf[i].nchan){
		p->nchan=_PICconf[i].nchan;
		if(nchan && p->nchan!=atoi(nchan)){
			werrstr("wrong NCHAN for this TYPE");
			goto Error;
		}
		if(p->chan==0) switch(p->nchan){
		case 1: p->chan="m"; break;
		case 3: p->chan="rgb"; break;
		}
	}
	else if(nchan)
		p->nchan=atoi(nchan);
	else if(p->chan)
		p->nchan=strlen(p->chan);
	else{
		werrstr("Neither CHAN nor NCHAN in header");
		goto Error;
	}
	if(p->chan==0 && p->nchan<=16)
		p->chan="????????????????"+16-p->nchan;
	window=picgetprop(p, "WINDOW");
	if(window==0){
		werrstr("No WINDOW in header");
		goto Error;
	}
	window=scanint(window, &x0); if(window==0) goto Error;
	window=scanint(window, &y0); if(window==0) goto Error;
	window=scanint(window, &x1); if(window==0) goto Error;
	window=scanint(window, &y1); if(window==0) goto Error;
	if(x0<x1){
		p->x=x0;
		p->width=x1-x0;
	}
	else{
		p->x=x1;
		p->width=x0-x1;
	}
	if(y0<y1){
		p->y=y0;
		p->height=y1-y0;
	}
	else{
		p->y=y1;
		p->height=y0-y1;
	}
	if(picgetprop(p, "CMAP")){
		p->cmap=malloc(3*256);
		if(p->cmap==0){
			werrstr("Can't allocate color map");
			goto Error;
		}
		for(i=3*256,lp=p->cmap;i!=0;i-=n,lp+=n){
			n=read(p->fd, lp, i);
			if(n<=0){
				werrstr("Can't read color map");
				goto Error;
			}
		}
	}
	else
		p->cmap=0;
	command=picgetprop(p, "COMMAND");
	if(command==0){
		sprint(line, "magically-create %s", file);
		command=line;
	}
	if(_PICcommand==0){
		_PICcommand=malloc(strlen(command)+1);
		if(_PICcommand)
			strcpy(_PICcommand, command);
	}
	else{
		_PICcommand=realloc(_PICcommand, strlen(_PICcommand)+strlen(command)+2);
		if(_PICcommand){
			strcat(_PICcommand, "\n");
			strcat(_PICcommand, command);
		}
	}
	return p;
}
