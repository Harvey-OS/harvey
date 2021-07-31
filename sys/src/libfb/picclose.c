#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
void picclose(PICFILE *f){
	register i;
	(*f->cl)(f);
	if(!(f->flags&PIC_NOCLOSE)) close(f->fd);
	if(f->cmap) free(f->cmap);
	for(i=0;i!=f->argc;i++) free(f->argv[i]);
	free((char *)f->argv);
	free((char *)f);
}
