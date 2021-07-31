#include        "lib9.h"
#include        "sys.h"
#include        "error.h"

Ref     mountid;

Rune    devchar[] = { '/', 'U', 'M', 'I', 'c', 'i', 'm', '|', 0 };
Dev     devtab[] =
{
	/* Root File System */
	{
		rootinit,         rootattach,   rootclone,        rootwalk,
		rootstat,         rootopen,     rootcreate,       rootclose,
		rootread,         devbread,	rootwrite,        devbwrite,
		rootremove,       rootwstat
	},
	/* Host File System */
	{
		fsinit,         fsattach,       fsclone,        fswalk,
		fsstat,         fsopen,         fscreate,       fsclose,
		fsread,         devbread,       fswrite,        devbwrite,
		fsremove,       fswstat
	},
	/* Mount */
	{
		mntinit,        mntattach,      mntclone,       mntwalk,
		mntstat,        mntopen,        mntcreate,      mntclose,
		mntread,        devbread,       mntwrite,       devbwrite,
		mntremove,      mntwstat
	},
	/* IP network support */
	{
		ipinit,         ipattach,       ipclone,        ipwalk,
		ipstat,         ipopen,         ipcreate,       ipclose,
		ipread,         devbread,       ipwrite,        devbwrite,
		ipremove,       ipwstat
	},
	/* Console device */
	{
		coninit,        conattach,      conclone,       conwalk,
		constat,        conopen,        concreate,      conclose,
		conread,        devbread,       conwrite,       devbwrite,
		conremove,      conwstat
	},
	/* draw */
	{
		devinit,	drawattach,	devclone,	drawwalk,
		drawstat,	drawopen,	devcreate,	drawclose,
		drawread,	devbread,	drawwrite,	devbwrite,
		devremove,	devwstat
	},
	/* Mouse */
	{
		mouseinit,	mouseattach,	mouseclone,       mousewalk,
		mousestat,	mouseopen,	mousecreate,      mouseclose,
		mouseread,	devbread,	mousewrite,       devbwrite,
		mouseremove,	mousewstat
	},
	/* pipe */
	{
		pipeinit,	pipeattach,	pipeclone,       pipewalk,
		pipestat,	pipeopen,	pipecreate,      pipeclose,
		piperead,	pipebread,	pipewrite,       pipebwrite,
		piperemove,	pipewstat
	},
};
