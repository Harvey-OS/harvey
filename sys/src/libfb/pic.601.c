/*
 * Type-specific code for TYPE=ccir601
 * Broken -- no reading or writing occurs
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
int _PRD601(PICFILE *f, void *vbuf){
	USED(vbuf);
	if(f->line==0){
		f->buf=malloc(f->width*2);
		if(f->buf==0){
			werrstr("Can't allocate buffer");
			return 0;
		}
	}
	if(f->line==f->height){
		werrstr("Read past end of picture");
		return 0;
	}
	/* read the file */
	f->line++;
	return 1;
}
int _PWR601(PICFILE *f, void *vbuf){
	USED(vbuf);
	if(f->line==0){
		f->buf=malloc(f->width*2);
		if(f->buf==0){
			werrstr("Can't allocate buffer");
			return 0;
		}
		_PWRheader(f);
	}
	if(f->line>=f->height){
		werrstr("Write past end of picture");
		return 0;
	}
	/* write the file */
	f->line++;
	return 1;
}
void _PCL601(PICFILE *f){
	if(f->buf) free(f->buf);
}
