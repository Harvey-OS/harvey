#include	"all.h"
#include	"ureg.h"
#include	<bootexec.h>

void	arginit(void);
int	diskid(void);

/*
 *  args passed by boot process
 */
int	_argc;
char	**_argv;
char	**_env;

/*
 *  environment passed to any kernel started by this kernel
 */
char	envbuf[512];
char	*env[8];

/*
 * environment that we use
 */
char	consname[NAMELEN];
char	debug[NAMELEN];
char	defaultpart[] = "boot";
char	bootpart[NAMELEN] = "boot";
char	bootline[2*NAMELEN];

void
main(void)
{
	struct	mipsexec hdr;
	long off;
	int i, c;

	delay(1000);
	icflush(0, 32*1024);
	tlbinit();
	arginit();
	screeninit(consname[0]);
	vecinit();
	devinit();
	kbdinit();
	print("SCSI boot\n");
	scsidebug = debug[1];
	scsireset();
	spllo();

	biogetdev(diskid());

Loop:
	strcpy(bootpart, defaultpart);
	while(getc(&kbdq) != -1)
		;

	getstr("boot partition", bootpart, sizeof bootpart, bootpart, 5);

	off = findpart(bootpart);
	if(off < 0){
		print("\npartition %s not found\n", bootpart);
		goto Loop;
	}

	sprint(bootline, "partboot");
	if(bioread(&hdr, off, sizeof hdr)){
		print("\ncan't read %s\n", bootpart);
		goto Loop;
	}
	off += (sizeof hdr + 0x04);	/* header is actually padded to 0x50 bytes */
	print("%ld", hdr.tsize);
	if(bioread((void *)hdr.text_start, off, hdr.tsize)){
		print("\ncan't read %s\n", bootpart);
		goto Loop;
	}
	off += hdr.tsize;
	print("+%ld", hdr.dsize);
	if(bioread((void *)hdr.data_start, off, hdr.dsize)){
		print("\ncan't read %s\n", bootpart);
		goto Loop;
	}
	print("+%ld\n", hdr.bsize);
	memset((void *)hdr.bss_start, 0, hdr.bsize);
	print("starting...");
	delay(1000);
	splhi();
	boot(hdr.mentry);
}

void
tlbinit(void)
{
	int i;

	for(i=0; i<NTLB; i++)
		puttlbx(i, KZERO | PTEPID(i), 0);
}

void
vecinit(void)
{
	ulong *p, *q;
	int size;

	p = (ulong*)EXCEPTION;
	q = (ulong*)vector80;
	for(size=0; size<4; size++)
		*p++ = *q++;
}

/*
 *  copy arguments passed by the boot kernel (or ROM) into a temporary buffer.
 *  we do this because the arguments are in memory that may be allocated
 *  to processes or kernel buffers.
 */
static char *
getenv(char *name)
{
	char **argv, *p;
	int n = strlen(name);

	for(argv = _env; *argv; argv++){
		p = *argv;
		if(strncmp(p, name, n)==0 && p[n] == '=')
			return p+n+1;
	}
	return 0;
}

char *envvars[]={
	"bootdevice",
	"bootserver",
	"console",
	"netaddr",
	0,
};

int
diskid(void)
{
	static int id = -1;
	char *p;

	if(id > 0)
		return id;
	/*
	 * dksd([[lun],id,])b
	 */
	id = 0;
	if(p = strchr(_argv[0], '(')){
		id = strtoul(p+1, 0, 10);
		if(p = strchr(p, ','))
			id += strtoul(p+1, 0, 10) << 3;
	}
	return id;
}

void
arginit(void)
{
	int i;
	char *p, *ep = envbuf, **envp = env;

	/*
	 *  get stuff from the environment
	 */
	for(i=0; envvars[i]; i++){
		if(!(p = getenv(envvars[i])))
			continue;
		*envp++ = ep;
		ep += sprint(ep, "%s=%s", envvars[i], p)+1;
	}
	*envp++ = ep;
	i = diskid();
	if(i & 7)
		sprint(ep, "bootdisk=#w%d.%d", i>>3, i&7);
	else
		sprint(ep, "bootdisk=#w%d", i>>3);
	*envp = 0;
	_env = env;

	p = getenv("console");
	if(p)
		strncpy(consname, p, NAMELEN-1);
	else
		strcpy(consname, "m");
	_argv[0] = bootline;
}

void
exit(void)
{
	int i;

	splhi();
	screenputs("\nexiting", 8);
	delay(2000);
	firmware();
}

int
sprint(char *buf, char *fmt, ...)
{
	return doprint(buf, buf+PRINTSIZE, fmt, (&fmt+1)) - buf;
}

int
print(char *fmt, ...)
{
	char buf[PRINTSIZE];
	int n;

	n = doprint(buf, buf+sizeof(buf), fmt, (&fmt+1)) - buf;
	screenputs(buf, n);
	return n;
}

void
panic(char *fmt, ...)
{
	char buf[PRINTSIZE];
	int i, n;

	screenputs("\npanic: ", 8);
	n = doprint(buf, buf+sizeof(buf), fmt, (&fmt+1)) - buf;
	screenputs(buf, n);

	for(i=0; i<1000000; i++)
		;
	firmware();
}

void
delay(int ms)
{
	ulong t, *p;
	int i;

	ms *= 7000;	/* experimentally determined */
	for(i=0; i<ms; i++)
		;
}
