#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	"io.h"

enum{
	Qrtc = 1,
	Qnvram,
    Qeeprom,
	Qnvram2,
	Qled,

	/* sccr */
	RTDIV=	1<<24,
	RTSEL=	1<<23,

	/* rtcsc */
	RTE=	1<<0,
	R38K=	1<<4,

	Nvoff=		0,	/* where usable EEPROM lives */
	Nvsize=		256,/*128x16=256x8*/
	Nvoff2=		8*1024,	/* where second usable nvram lives */
	Nvsize2=	4*1024,
};

static	QLock	rtclock;		/* mutex on clock operations */
static Lock nvrtlock;
static Lock eepromlock;

static Dirtab rtcdir[]={
	"rtc",		{Qrtc, 0},	12,	0666,
	"eeprom",	{Qeeprom, 0},	Nvsize,	0664,
	"nvram2",	{Qnvram2, 0},	Nvsize,	0664,
	"led",	{Qled, 0},	0,	0664,
};
#define	NRTC	(sizeof(rtcdir)/sizeof(rtcdir[0]))

extern void spiread(ulong addr, uchar *buf);
extern void spiwrite(ulong addr, uchar *buf);
extern void spiresult(void);
extern void spienwrite(void);
extern void spidiswrite(void);
extern void spiwriteall(uchar *buf);

static void
rtcreset(void)
{
	IMM *io;
	int n;

	io = m->iomem;
	io->rtcsck = KEEP_ALIVE_KEY;
	n = (RTClevel<<8)|RTE;
	if(m->oscclk == 5)
		n |= R38K;
	io->rtcsc = n;
	io->rtcsck = ~KEEP_ALIVE_KEY;
print("sccr=#%8.8lux plprcr=#%8.8lux\n", io->sccr, io->plprcr);
}

static Chan*
rtcattach(char *spec)
{
	return devattach('r', spec);
}

static int	 
rtcwalk(Chan *c, char *name)
{
	return devwalk(c, name, rtcdir, NRTC, devgen);
}

static void	 
rtcstat(Chan *c, char *dp)
{
	devstat(c, dp, rtcdir, NRTC, devgen);
}

static Chan*
rtcopen(Chan *c, int omode)
{
	omode = openmode(omode);
	switch(c->qid.path){
	case Qrtc:
	case Qled:
		if(strcmp(up->user, eve)!=0 && omode!=OREAD)
			error(Eperm);
		break;
	case Qeeprom:
    //case Qnvram:
	case Qnvram2:
		if(strcmp(up->user, eve)!=0)
			error(Eperm);
		break;
	}
	return devopen(c, omode, rtcdir, NRTC, devgen);
}

static void	 
rtcclose(Chan*)
{
}

static long	 
rtcread(Chan *c, void *buf, long n, vlong offset)
{
	ulong t,readaddr;
    uchar spibuf[2];
    int  index;
    
	if(c->qid.path & CHDIR)
		return devdirread(c, buf, n, rtcdir, NRTC, devgen);

	switch(c->qid.path){
	case Qrtc:
		t = m->iomem->rtc;
		n = readnum(offset, buf, n, t, 12);
		return n;
	case Qeeprom:
    //case Qnvram:
		if(offset >= Nvsize)
			return 0;
		t = offset;
		if(t + n > Nvsize)
			n = Nvsize - t;
		ilock(&eepromlock);
        
        /* read from eeprom through spi, two bytes a time*/

        index=0;
        if(t%2!=0){
          print("not starting from an even address\n");
          readaddr=(t-1)/2;
          spiread(readaddr,spibuf);
          memmove((uchar*)buf+index,(uchar *)spibuf+1,1);
          index++;
          readaddr++;
        }
        else
          readaddr=t/2;
        
        while(index<n){
            spiread(readaddr,spibuf);
            memmove((uchar*)buf+index,spibuf,1);
            if(index+1<n)
               memmove((uchar*)buf+index+1,(uchar*)spibuf+1,1);
            readaddr++;
            index+=2;
        }
        
/*        print("result of reading is \n");
        for(index=0;index<n;index++)
          print(" %ux ",((uchar*)buf)[index]);
        print("\n");
*/
		iunlock(&eepromlock);
		return n;
	case Qnvram2:
        print("ram 2\n");
		return 0;
		if(offset >= Nvsize2)
			return 0;
		t = offset;
		if(t + n > Nvsize2)
			n = Nvsize2 - t;
		ilock(&nvrtlock);
		memmove(buf, (uchar*)(NVRAMMEM + Nvoff2 + t), n);
		iunlock(&nvrtlock);
		return n;
	case Qled:
		if(offset >= 5)
			return 0;
		t = offset;
		if(t + n > 5)
			n = 5 - t;
		memmove(buf, "kitty" + t, n);
		return n;
	}
	error(Egreg);
	return 0;		/* not reached */
}

static long	 
rtcwrite(Chan *c, void *buf, long n, vlong offset)
{
	ulong secs;
	char *cp, *ep;
	IMM *io;
	ulong t,readaddr,writeaddr;
    int index;
    uchar spibuf[2];

	switch(c->qid.path){
	case Qrtc:
		if(offset!=0)
			error(Ebadarg);
		/*
		 *  read the time
		 */
		cp = ep = buf;
		ep += n;
		while(cp < ep){
			if(*cp>='0' && *cp<='9')
				break;
			cp++;
		}
		secs = strtoul(cp, 0, 0);
		/*
		 * set it
		 */
		io = ioplock();
		io->rtck = KEEP_ALIVE_KEY;
		io->rtc = secs;
		io->rtck = ~KEEP_ALIVE_KEY;
		iopunlock();
		return n;
	case Qeeprom:
    //case Qnvram:
		print("write %d to eeprom\n",n);
		if(offset >= Nvsize)
			return 0;
		t = offset;
		if(t + n > Nvsize)
			n = Nvsize - t;

		ilock(&eepromlock);
        index=0;
        spienwrite();
        /*if the writing addr starts at an odd address, we need to read in
          the higher byte first, combined it with the lower byte,
          and then write them out
        */
        if(t%2!=0){
          print("not starting from an even address\n");
          readaddr=(t-1)/2;
          spiread(readaddr,spibuf);
          memmove((uchar*)spibuf+1,(uchar*)buf+index,1);
          //spienwrite();
          spiwrite(readaddr,spibuf);
          //spidiswrite();
          index++;
          writeaddr=readaddr+1;
        }
        else
          writeaddr=t/2;

        while(index<n){
           if(index+1==n){
             //spienwrite();
             spiread(writeaddr,spibuf);
             //spidiswrite();
           }
           else
             memmove((uchar*)spibuf+1,(uchar*)buf+index+1,1);
           
           memmove((uchar*)spibuf,(uchar*)buf+index,1);
          // spienwrite();
           spiwrite(writeaddr,spibuf);
          // spidiswrite();
           writeaddr++;
           index+=2;
        }
          
        spidiswrite();
		//memmove((uchar*)(NVRAMMEM + Nvoff + offset), buf, n);
		iunlock(&eepromlock);
		return n;

	case Qnvram2:
        memmove((uchar*)spibuf,(uchar*)buf,2);
        spiwriteall(spibuf);
		return 0;
		if(offset >= Nvsize2)
			return 0;
		t = offset;
		if(t + n > Nvsize2)
			n = Nvsize2 - t;
		ilock(&nvrtlock);
		memmove((uchar*)(NVRAMMEM + Nvoff2 + offset), buf, n);
		iunlock(&nvrtlock);
		return n;
	case Qled:
		if(strncmp(buf, "on", 2) == 0)
			powerupled();
		else if(strncmp(buf, "off", 3) == 0)
			powerdownled();
		else
			error("unknown command");
		return n;
	}
	error(Egreg);
	return 0;		/* not reached */
}

long
rtctime(void)
{
	return m->iomem->rtc;
}

Dev rtcdevtab = {
	'r',
	"rtc",

	rtcreset,
	devinit,
	rtcattach,
	devclone,
	rtcwalk,
	rtcstat,
	rtcopen,
	devcreate,
	rtcclose,
	rtcread,
	devbread,
	rtcwrite,
	devbwrite,
	devremove,
	devwstat,
};
