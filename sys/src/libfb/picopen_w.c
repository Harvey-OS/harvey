#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
#define	NPROP	1024
_PRDerror(PICFILE *f, char *buf){
	USED(f, buf);
	return 0;
}
char cmdline[513];	/* from getflags */
PICFILE *picopen_w(char *file, char *type, int x, int y, int w, int h,
		   char *chan, char *argv[], char *cmap){
	int i;
	char prop[NPROP];
	PICFILE *p=(PICFILE *)malloc(sizeof(PICFILE));
	if(p==0) return 0;
	p->flags=PIC_NOCLOSE;
	p->argc=-1;
	p->cmap=0;
	if(strcmp(file, "OUT")==0)
		p->fd=1;
	else{
		p->fd=create(file, OWRITE, 0666);
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
		p->flags=0;
	}	
	p->argc=0;
	p->argv=(char **)malloc(sizeof(char *));
	p->argv[0]=0;
	for(i=0;_PICconf[i].type;i++)
		if(strcmp(type, _PICconf[i].type)==0) break;
	if(!_PICconf[i].type){
		_PICerror="Illegal TYPE";
		goto Error;
	}
	p->rd=_PRDerror;
	p->wr=_PICconf[i].wr;
	p->cl=_PICconf[i].cl;
	p->line=0;
	p->buf=0;
	picputprop(p, "TYPE", type);
	p->type=picgetprop(p, "TYPE");
	sprint(prop, "%d %d %d %d", x, y, x+w, y+h);
	picputprop(p, "WINDOW", prop);
	p->x=x;
	p->y=y;
	p->width=w;
	p->height=h;
	p->nchan=strlen(chan);
	if(_PICconf[i].nchan!=0 && _PICconf[i].nchan!=p->nchan){
		_PICerror="wrong NCHAN for this TYPE";
		goto Error;
	}
	sprint(prop, "%d", p->nchan);
	picputprop(p, "NCHAN", prop);
	picputprop(p, "CHAN", chan);
	p->chan=picgetprop(p, "CHAN");
	if(_PICcommand) picputprop(p, "COMMAND", _PICcommand);
	if(cmdline[0])
		strcpy(prop, cmdline);
	else if(argv){
		prop[0]='\0';
		for(i=0;argv[i];i++){
			if(i!=0) strcat(prop, " ");
			strcat(prop, argv[i]);
		}
	}
	else
		sprint(prop, "magically-create %s", file);
	picputprop(p, "COMMAND", prop);
	if(cmap){
		p->cmap=malloc(3*256);
		if(p->cmap==0){
			_PICerror="No memory for colormap";
			goto Error;
		}
		memcpy(p->cmap, cmap, 3*256);
		picputprop(p, "CMAP", "");
	}
	else
		p->cmap=0;
	return p;
}
void _PWRheader(PICFILE *p){
	register char **ap, *vp, *ep;
	int nlen;
	for(ap=p->argv;*ap;ap++){
		vp=strchr(*ap, '=')+1;
		nlen=vp-*ap;
		for(;;){
			write(p->fd, *ap, nlen);
			ep=strchr(vp, '\n');
			if(ep){
				write(p->fd, vp, ep+1-vp);
				vp=ep+1;
			}
			else{
				if(vp[0]) write(p->fd, vp, strlen(vp));
				write(p->fd, "\n", 1);
				break;
			}
		}
	}
	write(p->fd, "\n", 1);
	if(p->cmap)
		write(p->fd, p->cmap, 3*256);
}
