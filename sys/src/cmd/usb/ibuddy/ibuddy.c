#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"
#include "usbfs.h"
#include "ibuddy.h"

enum {
	/* Qids. Maintain order (relative to dirtabs structs) */
	Qroot	= 0,
	Qctl,
	Qmax,
};

typedef struct Dirtab Dirtab;
struct Dirtab {
	char	*name;
	int	mode;
};

static Dirtab dirtab[] = {
	[Qroot]	"/",	DMDIR|0555,
	[Qctl]	"ctl",	0664,
};

int ibuddydebug;

/*
 * Actions are written to the control end point
 * 8th byte holds the ibuddy's actions
 * WARNING: a bit set to 1 means OFF, 0 means ON
 * DO NOT activate IBRight and IBLeft simultaneosly
 * DO NOT activate IBOpen and IBClose simultaneosly
 * IF YOU DO SO, YOU  WILL DESTROY THE MOTORS
 * THE CODE IS EXTRA PARANOID REGARDING THAT
 */

uchar ibuddymsg[8]	=   {0x55, 0x53, 0x42, 0x43, 0x00, 0x40, 0x02, 0x00};
uchar ibuddysetup[8] = {0x22, 0x09, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00};

static int
usage(void)
{
	werrstr("usage: usb/ibuddy [-d]");
	return -1;
}

static int
checkmotors(uchar aux)
{	
	if((aux & (1<<IBRight)) == 0  &&  (aux & (1<<IBLeft)) == 0){
		dsprint(2, "ibuddy: checkmotors: fail in hips check\n");
		return -1;
	}
	if((aux & (1<<IBOpen)) == 0  &&  (aux & (1<<IBClose)) == 0){
		dsprint(2, "ibuddy: checkmotors: fail in wings check\n");
		return -1;
	}
	return 0;
}

void
ibuddyfatal(Ibuddy *bud)
{
	devctl(bud->dev, "detach");
	usbfsdel(&bud->fs);
	werrstr("io error");
}

/* do not use this on directly in order not to harm the motors */
static int
ibuddycmd(Ibuddy *bud, int command, int on)
{	
	int ret;
	uchar aux;

	if (command < 0 || command > 7)
		return -1;
	if(on)
		aux = bud->status & ~(1<<command);	/* status 1 means off */
	else
		aux = bud->status | (1<<command);
	if(checkmotors(aux) < 0){
		werrstr("ignoring harmful command: %b", aux);
		dsprint(2, "ibuddy: ibuddycmd: ignoring harmful command,"
			"  result would be: %ub\n", aux);
		return -1;
	}
	ibuddymsg[7] = aux;
	ret = usbcmd(bud->dev, Rh2d|Rclass|Riface, Rsetconf,  2, 1,  ibuddymsg, 8);
	ibuddymsg[7] = IBOff;
	if(ret < 0){
		ibuddyfatal(bud);
		return -1;
	}
	bud->status = aux;
	dsprint(2, "ibuddy: ibuddycmd: command done, status: %b\n", aux);
	return ret;
}


static int
ibuddylights(Ibuddy *bud, int light , int on)
{
	switch(light){
	case IBRed:
	case IBGreen:
	case IBBlue:
	case IBHeart:
		return ibuddycmd(bud, light, on);
	default:
		werrstr(Ebadctl);
		return -1;
	}
}

static int
ibuddyhips(Ibuddy *bud, int right)
{
	int ret;
	uchar aux;

	if(right < 0){
		werrstr(Ebadctl);
		return -1;
	}

	if(right){
		aux = bud->status | (1<<IBLeft);
		aux &=  ~(1<<IBRight);
	}else{
		aux = bud->status | (1<<IBRight);
		aux &= ~(1<<IBLeft);
	}
	if(checkmotors(aux) < 0){
		werrstr("ignoring a harmful command: %b", aux);
		dsprint(2, "ibuddy: ibuddyhips: ignoring harmful command,"
			"  result would be: %b\n", aux);
		return -1;
	}
	ibuddymsg[7] = aux;
	ret = usbcmd(bud->dev, Rh2d|Rclass|Riface, Rsetconf,  2, 1,  ibuddymsg, 8);
	ibuddymsg[7] = IBOff;
	if(ret < 0){
		ibuddyfatal(bud);
		return -1;
	}
	bud->status = aux;
	sleep(IBTwisttime);
	dsprint(2, "ibuddy: ibuddyhips: command done: %b\n", aux);
	return ret;
}

static int
ibuddywings(Ibuddy *bud, int open)
{
	int ret;
	uchar aux;

	if(open < 0){
		werrstr(Ebadctl);
		return -1;
	}
		
	if(open){
		aux = bud->status | (1<<IBClose);
		aux  &= ~(1<<IBOpen);
	}else{
		aux = bud->status | (1<<IBOpen);
		aux &= ~(1<<IBClose);
	}
	if(checkmotors(aux) < 0){
		werrstr("ignoring a harmful command for motors: %b", aux);
		dsprint(2, "ibuddy: ibuddywings: ignoring harmful command,"
			"  result would be: %b\n", aux);
		return -1;
	}
	ibuddymsg[7] = aux;
	ret = usbcmd(bud->dev, Rh2d|Rclass|Riface, Rsetconf,  2, 1,  ibuddymsg, 8);
	ibuddymsg[7] = IBOff;
	if(ret < 0){
		ibuddyfatal(bud);
		return -1;
	}
	bud->status = aux;
	sleep(IBTwisttime);
	dsprint(2, "ibuddy: ibuddywings: command done: %b\n", aux);
	return ret;
}

/* test the interface's  functions */
static void
ibuddytest(Ibuddy *bud)
{
	int i, j;

	for(i=0; i<20; i++){
		ibuddyhips(bud, 1);
		ibuddyhips(bud, 0);
	}
	sleep(5*1000);

	for(i=0; i<20; i++){
		ibuddywings(bud, 1);
		ibuddywings(bud, 0);
	}
	sleep(5*1000);

	for(i=0; i<2; i++)
		for(j=IBRed; j<IBHeart+1; j++){
			ibuddylights(bud, j, i%2);
			sleep(1000);
		}
}

static char *ibstate[] = {
	[IBRight] "hips right\n",
	[IBLeft] "hips left\n",
	[IBClose] "wings close\n",
	[IBOpen] "wings open\n",
	[IBRed] "red %s\n",
	[IBGreen] "green %s\n",		
	[IBBlue] "blue %s\n",
	[IBHeart] "heart %s\n",
};

enum {
	OFF,
	ON,
	NONE
};

static char *ledstate[] = {
	[OFF] "off",
	[ON] "on",
};

static char * 
ibuddydumpstate(Ibuddy *bud, char * buf, int len)
{
	int w, i, st, hips;

	w = 0;
	hips = 0;

	for(i = 0; i < IBOpen; i++){
		if(bud->status & (1<< i)){
			w += snprint(buf + w, len - w, ibstate[i]);
			hips++;
		}
		if(i == IBLeft && hips == 0)
			w += snprint(buf + w, len - w, "hips none\n");
	}
	for(i = IBRed; i < IBHeart; i++){
		if(bud->status & (1<<i))
			st = OFF;	/* inverted logic */
		else
			st = ON;
		w += snprint(buf + w, len - w, ibstate[i], ledstate[st]);
	}
	return buf + w;
}

static int
dwalk(Usbfs *fs, Fid *fid, char *name)
{
	Qid qid;

	qid = fid->qid;
	if((qid.type & QTDIR) == 0){
		werrstr("walk in non-directory");
		return -1;
	}
	if(qid.path != (Qroot | fs->qid)){
		werrstr("qid must be Qroot!");
		return -1;
	}
	if(strcmp(name, "..") == 0 || strcmp(name, ".") == 0){
		/*can only be the root */
		fid->qid.path = Qroot | fs->qid;
		fid->qid.vers = 0;
		fid->qid.type = QTDIR;
		return 0;
	}
	if(strcmp(name, "ctl") == 0){
		qid.path = Qctl | fs->qid;
		qid.vers = 0;
		qid.type = dirtab[Qctl].mode >> 24;
		fid->qid = qid;
		return 0;
	}
	werrstr(Enotfound);
	return -1;
}

static int
dstat(Usbfs *fs, Qid qid, Dir *d)
{
	int path;
	Ibuddy *bud;
	Dirtab *t;

	path = qid.path & ~fs->qid;
	t = &dirtab[path];
	d->qid.path = path;
	d->qid.type = t->mode >> 24;
	d->mode = t->mode;
	/* if it's the root, the name is the FS' name */
	bud = fs->aux;
	if(strcmp(t->name, "/") == 0)
		d->name = t->name;
	else
		snprint(d->name, Namesz, t->name, bud->fs.name);
	d->length = 0;
	d->qid.path |= fs->qid;
	return 0;
}

static int
dopen(Usbfs *fs, Fid *fid, int)
{
	ulong path;

	path = fid->qid.path & ~fs->qid;
	switch(path){	
	case Qroot:
		dsprint(2, "ibuddy, opened root\n");
		break;	
	case Qctl:
		dsprint(2, "ibuddy, opened ctl\n");
		break;
	default:
		werrstr(Enotfound);
		return -1;
	}
	return 0;
}

static void
filldir(Usbfs *fs, Dir *d, Dirtab *tab, int i)
{
	d->qid.path = i | fs->qid;
	d->mode = tab->mode;
	if((d->mode & DMDIR) != 0)
		d->qid.type = QTDIR;
	else
		d->qid.type = QTFILE;
	d->name = tab->name;
}

static int
dirgen(Usbfs *fs, Qid, int i, Dir *d, void *)
{
	Dirtab *tab;

	i++;				/* skip root */
	if(i < nelem(dirtab))
		tab = &dirtab[i];
	else
		return -1;
	filldir(fs, d, tab, i);
	return 0;
}

static long
dread(Usbfs *fs, Fid *fid, void *data, long count, vlong offset)
{
	ulong path;
	char *buf, *err;	/* change */
	char *e;
	Qid q;
	Ibuddy *bud;

	q = fid->qid;
	path = fid->qid.path & ~fs->qid;
	bud = fs->aux;

	buf = emallocz(255, 1);
	err = emallocz(255, 1);
	qlock(bud);
	switch(path){
	case Qroot:
		count = usbdirread(fs, q, data, count, offset, dirgen, nil);
		break;
	case Qctl:
		if(offset != 0){
			count = 0;
			break;
		}
		e = ibuddydumpstate(bud, buf, 255);
		count = usbreadbuf(data, count, 0, buf, e - buf);
		break;
	default:
		werrstr(Ebadfid);
		count = -1;
	}
	qunlock(bud);
	free(err);
	free(buf);
	return count;
}

enum{
	Ledtype,
	Sidetype,
	Flaptype,
	Ntypes,
};

static char *valstr[] = {
	[Ledtype] "off",
	[Sidetype] "right",
	[Flaptype] "open",

	[Ledtype+Ntypes] "on",
	[Sidetype+Ntypes] "left",
	[Flaptype+Ntypes] "closed",
};

static int
value(char *name, int type)
{
	if(strncmp(name, valstr[type], strlen(valstr[type])) == 0)
		return 0;
	 if(strncmp(name, valstr[type], strlen(valstr[type+Ntypes])) == 0)
		return 1;
	return -1;
}

static int
ibuddyprocessline(Ibuddy *bud, char *cmd)
{
	char *tok[3];
	int n, i, l, v;
	int light;
	
	n = tokenize(cmd, tok, 3);
	if(n != 2)
		return -1;
	
	for(i = IBRed; i < IBHeart; i++){
		l = strchr(ibstate[i], ' ') - ibstate[i];
		if(strncmp(tok[0], ibstate[i], l) == 0){
				light = i;
				v = value(tok[1], Ledtype);
				return ibuddylights(bud, light, v);	
		}
	}

	if(strcmp(tok[0], "hips") == 0){
		v = value(tok[1], Sidetype);
		return ibuddyhips(bud, v);
	}
	if(strcmp(tok[0], "wings") == 0){
		v = value(tok[1], Flaptype);
		return ibuddywings(bud, v);
	}
	werrstr(Ebadctl);
	return -1;
}

static int
ibuddyprocesscmds(Ibuddy *bud, char *cmd)
{
	char *tok[6];
	int n;
	int i;

	n = gettokens(cmd, tok, 6, "\n");
	for(i=0; i < n; i++){
		if(ibuddyprocessline(bud, tok[i]) < 0)
			return -1;
	}
	return 0;
}

static long
dwrite(Usbfs *fs, Fid *fid, void *buf, long count, vlong)
{
	ulong path;
	char *cmd;
	Ibuddy *bud;
	bud = fs->aux;
	path = fid->qid.path & ~fs->qid;

	qlock(bud);
	switch(path){
	case Qctl:
		cmd = emallocz(count+1, 1);
		memmove(cmd, buf, count);
		cmd[count] = 0;
		if(ibuddyprocesscmds(bud, cmd) < 0){
			/* errstr written in the call */
			qunlock(bud);
			free(cmd);
			return -1;
		}
		free(cmd);
		break;
	default:
		werrstr(Eperm);
		count =  -1;
	}
	qunlock(bud);
	return count;
}

static Usbfs ibuddylfs = {
	.walk =	dwalk,
	.open =	dopen,
	.read =	dread,
	.write=	dwrite,
	.stat =	dstat,
};

static void
ibuddydevfree(void *a)
{
	Ibuddy *bud = a;

	if(bud == nil)
		return;
	dsprint(2, "ibuddy: ibuddydevfree: freeing\n");
	bud->dev = nil;
	free(bud);
}

int
ibuddymain(Dev *dev, int argc, char* argv[])
{
	Ibuddy *bud;
	int res;

	ARGBEGIN{
	case 'd':
		ibuddydebug++;
		break;
	default:
		return usage();
	}ARGEND
	if(argc != 0)
		return usage();

	bud = dev->aux = emallocz(sizeof(Ibuddy), 1);
	bud->dev = dev;
	dev->free = ibuddydevfree;

	/* reset the ibuddy */
	res = usbcmd(dev, Rh2d|Rclass|Riface, Rsetconf,   2, 1, ibuddysetup, 8);
	if(res < 0){
		dsprint(2, "ibuddy: ibuddymain: reset usbcmd returned: %d\n", res);
		return -1;
	}
	/* set to: right, close wings, and lights on */
	bud->status = IBOff;	
	ibuddywings(bud, 0);
	ibuddyhips(bud, 1);
	ibuddylights(bud, IBRed, 0);
	ibuddylights(bud, IBGreen, 0);
	ibuddylights(bud, IBBlue, 0);
	ibuddylights(bud, IBHeart, 0);

	bud->fs = ibuddylfs;
	snprint(bud->fs.name, sizeof(bud->fs.name), "ibuddy%d", dev->id);
	bud->fs.dev = dev;
	incref(dev);
	bud->fs.aux = bud;
	usbfsadd(&bud->fs);
	return 0;
}

