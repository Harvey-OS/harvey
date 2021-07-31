#include <u.h>
#include <libc.h>
#include <libg.h>
#include <panel.h>
Panel *root;
void done(Panel *p, int buttons){
	USED(p, buttons);
	exits(0);
}
