#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ndb.h>

#include "vga.h"

static Ndb*
dbopen(char *dbname)
{
	Ndb *db;

	if((db = ndbopen(dbname)) == 0)
		error("dbopen: %s: %r\n", dbname);
	return db;
}

static Ctlr*
addctlr(Vga *vga, char *val)
{
	Ctlr **ctlr;
	char name[NAMELEN+1], *p;
	int i;

	/*
	 * A controller name may have an extension on the end
	 * following a '-' which can be used as a speed grade or
	 * subtype.  Do the match without the extension.
	 * The linked copy of the controller struct gets the
	 * full name with extension.
	 */
	strncpy(name, val, NAMELEN);
	name[NAMELEN] = 0;
	if(p = strchr(name, '-'))
		*p = 0;

	for(i = 0; ctlrs[i]; i++){
		if(strcmp(ctlrs[i]->name, name))
			continue;
		for(ctlr = &vga->link; *ctlr; ctlr = &((*ctlr)->link))
			;
		*ctlr = alloc(sizeof(Ctlr));
		**ctlr = *ctlrs[i];
		strncpy((*ctlr)->name, val, NAMELEN);
		return *ctlr;	
	}

	error("don't know how to programme a \"%s\" ctlr\n", val);
	return 0;
}

int
dbctlr(char *name, Vga *vga)
{
	Ndb *db;
	Ndbs s;
	Ndbtuple *t, *tuple;
	char bios[128];
	char *string;
	long offset;
	int len;

	db = dbopen(name);

	for(tuple = ndbsearch(db, &s, "ctlr", ""); tuple; tuple = ndbsnext(&s, "ctlr", "")){
		for(t = tuple->entry; t; t = t->entry){
			if((offset = strtoul(t->attr, 0, 0)) == 0)
				continue;

			string = t->val;
			len = strlen(string);
			readbios(bios, len, offset);
			if(strncmp(bios, string, len) == 0){
				for(t = tuple->entry; t; t = t->entry){
					if(strcmp(t->attr, "ctlr") == 0)
						vga->ctlr = addctlr(vga, t->val);
					else if(strcmp(t->attr, "ramdac") == 0)
						vga->ramdac = addctlr(vga, t->val);
					else if(strcmp(t->attr, "clock") == 0)
						vga->clock = addctlr(vga, t->val);
					else if(strcmp(t->attr, "hwgc") == 0)
						vga->hwgc = addctlr(vga, t->val);
					else if(strcmp(t->attr, "link") == 0)
						addctlr(vga, t->val);
				}
				ndbfree(tuple);
				ndbclose(db);
				return 1;
			}
		}

		ndbfree(tuple);
	}

	ndbclose(db);
	return 0;
}

static int
dbmonitor(Ndb *db, Mode *mode, char *type, char *size)
{
	Ndbs s;
	Ndbtuple *t, *tuple;
	char *p, attr[NAMELEN+1], val[NAMELEN+1];
	int x;

	memset(mode, 0, sizeof(Mode));
	strcpy(attr, type);
	strcpy(val, size);
buggery:
	if((tuple = ndbsearch(db, &s, attr, val)) == 0)
		return 0;

	if(mode->x == 0 && ((mode->x = strtol(val, &p, 0)) == 0 || *p++ != 'x'))
		return 0;
	if(mode->y == 0 && ((mode->y = strtol(p, &p, 0)) == 0 || *p++ != 'x'))
		return 0;
	if(mode->z == 0 && ((mode->z = strtol(p, &p, 0)) == 0))
		return 0;

	for(t = tuple->entry; t; t = t->entry){
		if(strcmp(t->attr, "clock") == 0 && mode->frequency == 0)
			mode->frequency = strtod(t->val, 0)*1000000;
		else if(strcmp(t->attr, "ht") == 0 && mode->ht == 0)
			mode->ht = strtol(t->val, 0, 0);
		else if(strcmp(t->attr, "shb") == 0 && mode->shb == 0)
			mode->shb = strtol(t->val, 0, 0);
		else if(strcmp(t->attr, "ehb") == 0 && mode->ehb == 0)
			mode->ehb = strtol(t->val, 0, 0);
		else if(strcmp(t->attr, "vt") == 0 && mode->vt == 0)
			mode->vt = strtol(t->val, 0, 0);
		else if(strcmp(t->attr, "vrs") == 0 && mode->vrs == 0)
			mode->vrs = strtol(t->val, 0, 0);
		else if(strcmp(t->attr, "vre") == 0 && mode->vre == 0)
			mode->vre = strtol(t->val, 0, 0);
		else if(strcmp(t->attr, "hsync") == 0)
			mode->hsync = *t->val;
		else if(strcmp(t->attr, "vsync") == 0)
			mode->vsync = *t->val;
		else if(strcmp(t->attr, "interlace") == 0)
			mode->interlace = *t->val;
		else{
			strcpy(attr, t->attr);
			strcpy(val, t->val);
			ndbfree(tuple);
			goto buggery;
		}
	}
	ndbfree(tuple);

	if((x = strtol(size, &p, 0)) == 0 || x != mode->x || *p++ != 'x')
		return 0;
	if((x = strtol(p, &p, 0)) == 0 || x != mode->y || *p++ != 'x')
		return 0;
	if((x = strtol(p, &p, 0)) == 0 || x != mode->z)
		return 0;

	return 1;
}

Mode*
dbmode(char *name, char *type, char *size)
{
	Ndb *db;
	Ndbs s;
	Ndbtuple *t, *tuple;
	Mode *mode;
	char attr[NAMELEN+1];

	db = dbopen(name);
	mode = alloc(sizeof(Mode));
	strcpy(attr, type);

	/*
	 * Look for the attr=size entry.
	 */
	if(dbmonitor(db, mode, attr, size)){
		strcpy(mode->type, type);
		strcpy(mode->size, size);
		ndbclose(db);
		return mode;
	}

	/*
	 * Not found. Look for an attr="" entry and then
	 * for an alias=attr within.
	 */
buggery:
	for(tuple = ndbsearch(db, &s, attr, ""); tuple; tuple = ndbsnext(&s, attr, "")){
		for(t = tuple->entry; t; t = t->entry){
			if(strcmp(t->attr, "alias"))
				continue;
			strcpy(attr, t->val);
			if(dbmonitor(db, mode, attr, size)){
				strcpy(mode->type, type);
				strcpy(mode->size, size);
				ndbfree(tuple);
				ndbclose(db);
				return mode;
			}

			/*
			 * Found an alias but no match for size,
			 * restart looking for attr="" with the
			 * new attr.
			 */
			ndbfree(tuple);
			goto buggery;
		}
		ndbfree(tuple);
	}

	free(mode);
	ndbclose(db);
	return 0;
}

void
dbdumpmode(Mode *mode)
{
	verbose("dbdumpmode\n");

	print("type=%s, size=%s\n", mode->type, mode->size);
	print("frequency=%d\n", mode->frequency);
	print("x=%d (0x%X), y=%d (0x%X), z=%d (0x%X)\n",
		mode->x, mode->x, mode->y,  mode->y, mode->z, mode->z);
	print("ht=%d (0x%X), shb=%d (0x%X), ehb=%d (0x%X)\n",
		mode->ht, mode->ht, mode->shb, mode->shb, mode->ehb, mode->ehb);
	print("vt=%d (0x%X), vrs=%d (0x%X), vre=%d (0x%X)\n",
		mode->vt, mode->vt, mode->vrs, mode->vrs, mode->vre, mode->vre);
	print("hsync=%d, vsync=%d, interlace=%d\n",
		mode->hsync, mode->vsync, mode->interlace);
}
