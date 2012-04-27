#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ndb.h>

#include "pci.h"
#include "vga.h"

static Ndb*
dbopen(char* dbname)
{
	Ndb *db;

	if((db = ndbopen(dbname)) == 0)
		error("dbopen: %s: %r\n", dbname);
	return db;
}

static void
addattr(Attr** app, Ndbtuple* t)
{
	Attr *attr, *l;

	attr = alloc(sizeof(Attr));
	attr->attr = alloc(strlen(t->attr)+1);
	strcpy(attr->attr, t->attr);
	attr->val = alloc(strlen(t->val)+1);
	strcpy(attr->val, t->val);

	for(l = *app; l; l = l->next)
		app = &l->next;
	*app = attr;
}

char*
dbattr(Attr* ap, char* attr)
{
	while(ap){
		if(strcmp(ap->attr, attr) == 0)
			return ap->val;
		ap = ap->next;
	}

	return 0;
}

static Ctlr*
addctlr(Vga* vga, char* val)
{
	Ctlr **ctlr;
	char name[Namelen+1], *p;
	int i;

	/*
	 * A controller name may have an extension on the end
	 * following a '-' which can be used as a speed grade or
	 * subtype.  Do the match without the extension.
	 * The linked copy of the controller struct gets the
	 * full name with extension.
	 */
	strncpy(name, val, Namelen);
	name[Namelen] = 0;
	if(p = strchr(name, '-'))
		*p = 0;

	for(i = 0; ctlrs[i]; i++){
		if(strcmp(ctlrs[i]->name, name))
			continue;
		for(ctlr = &vga->link; *ctlr; ctlr = &((*ctlr)->link))
			;
		*ctlr = alloc(sizeof(Ctlr));
		**ctlr = *ctlrs[i];
		strncpy((*ctlr)->name, val, Namelen);
		return *ctlr;	
	}

	fprint(2, "dbctlr: unknown controller \"%s\" ctlr\n", val);
	return 0;
}

int
dbbios(Vga *vga, Ndbtuple *tuple)
{
	char *bios, *p, *string;
	int len;
	long offset, offset1;
	Ndbtuple *t;

	for(t = tuple->entry; t; t = t->entry){
		if((offset = strtol(t->attr, 0, 0)) == 0)
			continue;

		string = t->val;
		len = strlen(string);

		if(p = strchr(t->attr, '-')) {
			if((offset1 = strtol(p+1, 0, 0)) < offset+len)
				continue;
		} else
			offset1 = offset+len;

		if(vga->offset) {
			if(offset > vga->offset || vga->offset+len > offset1)
				continue;
			offset = vga->offset;
			offset1 = offset+len;
		}

		for(; offset+len<=offset1; offset++) {
			if(vga->bios)
				bios = vga->bios;
			else
				bios = readbios(len, offset);
			if(strncmp(bios, string, len) == 0){
				if(vga->bios == 0){
					vga->bios = alloc(len+1);
					strncpy(vga->bios, bios, len);
				}
				addattr(&vga->attr, t);
				return 1;
			}
		}
	}
	return 0;
}

int
dbpci(Vga *vga, Ndbtuple *tuple)
{
	int did, vid;
	Ndbtuple *t, *td;
	Pcidev *pci;

	for(t = tuple->entry; t; t = t->entry){
		if(strcmp(t->attr, "vid") != 0 || (vid=atoi(t->val)) == 0)
			continue;
		for(td = t->line; td != t; td = td->line){
			if(strcmp(td->attr, "did") != 0)
				continue;
			if(strcmp(td->val, "*") == 0)
				did = 0;
			else if((did=atoi(td->val)) == 0)
				continue;
			for(pci=nil; pci=pcimatch(pci, vid, did);)
				if((pci->ccru>>8) == 3)
					break;
			if(pci == nil)
				continue;
			vga->pci = pci;
			addattr(&vga->attr, t);
			addattr(&vga->attr, td);
			return 1;
		}
	}
	return 0;
}

static void
save(Vga *vga, Ndbtuple *tuple)
{
	Ctlr *c;
	Ndbtuple *t;

	for(t = tuple->entry; t; t = t->entry){
		if(strcmp(t->attr, "ctlr") == 0){
			vga->ctlr = addctlr(vga, t->val);
			if(strcmp(t->val, "vesa") == 0)
				vga->vesa = vga->ctlr;
		}else if(strcmp(t->attr, "ramdac") == 0)
			vga->ramdac = addctlr(vga, t->val);
		else if(strcmp(t->attr, "clock") == 0)
			vga->clock = addctlr(vga, t->val);
		else if(strcmp(t->attr, "hwgc") == 0)
			vga->hwgc = addctlr(vga, t->val);
		else if(strcmp(t->attr, "link") == 0){
			c = addctlr(vga, t->val);
			if(strcmp(t->val, "vesa") == 0)
				vga->vesa = c;
		}else if(strcmp(t->attr, "linear") == 0)
			vga->linear = strtol(t->val, 0, 0);
		else if(strcmp(t->attr, "membw") == 0)
			vga->membw = strtol(t->val, 0, 0)*1000000;
		else if(strcmp(t->attr, "vid")==0 || strcmp(t->attr, "did")==0)
			{}
		else if(strtol(t->attr, 0, 0) == 0)
			addattr(&vga->attr, t);
	}
}

int
dbctlr(char* name, Vga* vga)
{
	Ndb *db;
	Ndbs s;
	Ndbtuple *tuple;
	Ndbtuple *pcituple;

	db = dbopen(name);

	/*
	 * Search vgadb for a matching BIOS string or PCI id.
	 * If we have both, the BIOS string wins.
	 */
	pcituple = nil;
	for(tuple = ndbsearch(db, &s, "ctlr", ""); tuple; tuple = ndbsnext(&s, "ctlr", "")){
		if(!pcituple && dbpci(vga, tuple))
			pcituple = tuple;
		if(dbbios(vga, tuple)){
			save(vga, tuple);
			if(pcituple && pcituple != tuple)
				ndbfree(pcituple);
			ndbfree(tuple);
			ndbclose(db);
			return 1;
		}
		if(tuple != pcituple)
			ndbfree(tuple);
	}

	if(pcituple){
		save(vga, pcituple);
		ndbfree(pcituple);
	}
	ndbclose(db);
	if(pcituple)
		return 1;
	return 0;
}

static int
dbmonitor(Ndb* db, Mode* mode, char* type, char* size)
{
	Ndbs s;
	Ndbtuple *t, *tuple;
	char *p, attr[Namelen+1], val[Namelen+1], buf[2*Namelen+1];
	int clock, x, i;

	/*
	 * Clock rate hack.
	 * If the size is 'XxYxZ@NMHz' then override the database entry's
	 * 'clock=' with 'N*1000000'.
	 */
	clock = 0;
	strcpy(buf, size);
	if(p = strchr(buf, '@')){
		*p++ = 0;
		if((clock = strtol(p, &p, 0)) && strcmp(p, "MHz") == 0)
			clock *= 1000000;
	}

	memset(mode, 0, sizeof(Mode));

	if((p = strchr(buf, 'x')) && (p = strchr(p+1, 'x'))){
		*p++ = 0;
		mode->z = atoi(p);
	}

	strcpy(attr, type);
	strcpy(val, buf);

	if(p = ndbgetvalue(db, &s, attr, "", "videobw", nil)){
		mode->videobw = atol(p)*1000000UL;
		free(p);
	}

	if(mode->x == 0 && ((mode->x = strtol(val, &p, 0)) == 0 || *p++ != 'x'))
		return 0;
	if(mode->y == 0 && (mode->y = strtol(p, &p, 0)) == 0)
		return 0;
	i = 0;
buggery:
	if((tuple = ndbsearch(db, &s, attr, val)) == 0)
		return 0;

	for(t = tuple->entry; t; t = t->entry){
		if(strcmp(t->attr, "clock") == 0 && mode->frequency == 0)
			mode->frequency = strtod(t->val, 0)*1000000;
		else if(strcmp(t->attr, "defaultclock") == 0 && mode->deffrequency == 0)
			mode->deffrequency = strtod(t->val, 0)*1000000;
		else if(strcmp(t->attr, "ht") == 0 && mode->ht == 0)
			mode->ht = strtol(t->val, 0, 0);
		else if(strcmp(t->attr, "shb") == 0 && mode->shb == 0)
			mode->shb = strtol(t->val, 0, 0);
		else if(strcmp(t->attr, "ehb") == 0 && mode->ehb == 0)
			mode->ehb = strtol(t->val, 0, 0);
		else if(strcmp(t->attr, "shs") == 0 && mode->shs == 0)
			mode->shs = strtol(t->val, 0, 0);
		else if(strcmp(t->attr, "ehs") == 0 && mode->ehs == 0)
			mode->ehs = strtol(t->val, 0, 0);
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
		else if(strcmp(t->attr, "include") == 0 /*&& strcmp(t->val, val) != 0*/){
			strcpy(attr, t->attr);
			strcpy(val, t->val);
			ndbfree(tuple);
			if(i++ > 5)
				error("dbmonitor: implausible include depth at %s=%s\n", attr, val);
			goto buggery;
		}
		else if(strcmp(t->attr, "include") == 0){
			print("warning: bailed out of infinite loop in attr %s=%s\n", attr, val);
		}
		else
			addattr(&mode->attr, t);
	}
	ndbfree(tuple);

	if((x = strtol(size, &p, 0)) == 0 || x != mode->x || *p++ != 'x')
		return 0;
	if((x = strtol(p, &p, 0)) == 0 || x != mode->y || *p++ != 'x')
		return 0;
	if((x = strtol(p, &p, 0)) == 0 || x != mode->z)
		return 0;

	if(clock)
		mode->frequency = clock;

	return 1;
}

Mode*
dbmode(char* name, char* type, char* size)
{
	Ndb *db;
	Ndbs s;
	Ndbtuple *t, *tuple;
	Mode *mode;
	char attr[Namelen+1];
	ulong videobw;

	db = dbopen(name);
	mode = alloc(sizeof(Mode));
	strcpy(attr, type);

	videobw = 0;
	/*
	 * Look for the attr=size entry.
	 */
	if(dbmonitor(db, mode, attr, size)){
		strcpy(mode->type, type);
		strcpy(mode->size, size);
		ndbclose(db);
		return mode;
	}

	if(mode->videobw && videobw == 0)	/* we at least found that; save it away */
		videobw = mode->videobw;

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
				if(videobw)
					mode->videobw = videobw;
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
dbdumpmode(Mode* mode)
{
	Attr *attr;

	Bprint(&stdout, "dbdumpmode\n");

	Bprint(&stdout, "type=%s, size=%s\n", mode->type, mode->size);
	Bprint(&stdout, "frequency=%d\n", mode->frequency);
	Bprint(&stdout, "x=%d (0x%X), y=%d (0x%X), z=%d (0x%X)\n",
		mode->x, mode->x, mode->y,  mode->y, mode->z, mode->z);
	Bprint(&stdout, "ht=%d (0x%X), shb=%d (0x%X), ehb=%d (0x%X)\n",
		mode->ht, mode->ht, mode->shb, mode->shb, mode->ehb, mode->ehb);
	Bprint(&stdout, "shs=%d (0x%X), ehs=%d (0x%X)\n",
		mode->shs, mode->shs, mode->ehs, mode->ehs);
	Bprint(&stdout, "vt=%d (0x%X), vrs=%d (0x%X), vre=%d (0x%X)\n",
		mode->vt, mode->vt, mode->vrs, mode->vrs, mode->vre, mode->vre);
	Bprint(&stdout, "hsync=%d, vsync=%d, interlace=%d\n",
		mode->hsync, mode->vsync, mode->interlace);

	for(attr = mode->attr; attr; attr = attr->next)
		Bprint(&stdout, "mode->attr: %s=%s\n", attr->attr, attr->val);
}
