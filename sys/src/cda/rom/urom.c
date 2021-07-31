#ifdef PLAN9
#define min(x, y) ((x) < (y)) ? (x) : (y)
#define exit(x) exits(x ? "abnormal" : 0)
#else
# include <sys/ttyio.h>
# include <sys/filio.h>
#endif
# include <stdio.h>

#ifdef PLAN9
# define PROMFILE "dk!unisite"
# define CODEFILE "/sys/lib/cda/urom.codes"
# define TTYNAME "/dev/cons"
#else
# define PROMFILE "/cs/dk!unisite"
# define CODEFILE "/usr/ucds/lib/drom.codes"
# define TTYNAME "/dev/tty"
#endif

# define	SIZE	262144	/* no longer needs one extra */
# define SMALL 128
# define STX 02
# define ETX 03
# define DC1 0x11
# define DC3 0x13
# define BADCNT	1
# define BADSUM	2
# define BADADD	4
# define MAXCODES 5000

extern char *errstr;
FILE	*fpr, *fpw, *ftty;
int	dp;
int	junk;
unsigned int data[ SIZE ], *dap, *dapmax;
unsigned inv = 0;
char	icvt[] = "%o";
char	ocvt[] = "%#o\n";

char c;

#ifdef PLAN9
#define MAXNAME 64
struct trans {
	char	type[MAXNAME];
	char	maker[MAXNAME];
	unsigned char Tflg;
	unsigned int code;
} tab[MAXCODES];
#else
struct trans {
	char	type[11];
	unsigned char Tflg;
	unsigned code;
} tab[MAXCODES];
#endif

int sum, maxa, osum;
int Vflg = 0;
int vflg;
unsigned char Tflg = 1;
unsigned code, codeset = 0;
int Bflg = 0;		/*binary file flag*/
int bflg = 0;		/*blankcheck flag*/
int xfmt = 0x050;	/* code xon xoff,ascii hex space (pages a2 a4) */
int csflg = 0;		/* checksum flag */
int nflg = 0;		/*no, echo family&pinout*/
int ntimes = 1;		/* make ntimes copies */
char * mcode = (char *) 0;
char * tcode = (char *) 0;
char * inhib = (char *) 0;
void gettype(), cmd(), readcodes();

void
main (argc,argv)
int argc;
char *argv[];

{
	FILE *dcs;
	unsigned bdmsk;
	int	r, n, err;
	int	ndata, width, v, fsn;
	int start = 0;		/*address offset*/
	int wflg = 0;		/*write flag*/
	int iflg = 0;		/*1's comp i/o flag*/

	dcs = fopen(CODEFILE, "r");
	if (dcs == (FILE *) NULL) {
		fprintf(stderr, "cannot open device code file %s\n", CODEFILE);
		exit(1);
	} 
	readcodes(dcs);

	/* scan command line for arguments */

	while( argc > 1 ){
		if( argv[1][0] != '-' ) {
			gettype(argv[1]);
			goto nexta;
		}
	    while(c = *++argv[1]) {
		switch (c) {
		case 'b':
			bflg++;
			break;
		case 'B':
			Bflg = 1;
			break;
		case 'C':
			icvt[1] = 'c';
			ocvt[1] = 'c';
			ocvt[2] = 0;
			break;
		case 'c':
			if(codeset) {
				fprintf(stderr,"code already set, Goodbye");
				exit(1);
			}
			if(*++argv[1] == '\0') {++argv;--argc;}
			if(sscanf(argv[1],"%x",&code) != 1) {
				fprintf(stderr,"-c!%%x\n");exit(1);
			}
			codeset++;
			goto nexta;
		case 'D':
		case 'd':
			icvt[1] = 'd';
			ocvt[2] = 'd';
			break;
		case 'f':
			if(*++argv[1] == '\0') {++argv;--argc;}
			if(sscanf(argv[1],"%x",&xfmt) != 1) {
				fprintf(stderr,"-f!%%x\n");exit(1);
			}
			icvt[1] = 'c';
			ocvt[1] = 'c';
			ocvt[2] = 0;
			goto nexta;
		case 'i':
			iflg++;			/* invert on in/out */
			break;

		case 'I':			/* inhibit */
			if(*++argv[1] == '\0') {++argv;--argc;}
			inhib = argv[1];
			goto nexta;
			
		case 'J':
			xfmt = 0x091;
			icvt[1] = 'c';
			ocvt[1] = 'c';
			ocvt[2] = 0;
			break;
		case 'm':
			if(*++argv[1] == '\0') {++argv;--argc;}
			mcode = argv[1];
			goto nexta;

		case 'n':
			nflg++;			/* no */
			break;
		case 'N':
			if(*++argv[1] == '\0') {++argv;--argc;}
			if(sscanf(argv[1],"%d",&ntimes) != 1) {
				fprintf(stderr,"-N!%%d\n");exit(1);
			}
			goto nexta;
		case 'O':
		case 'o':
			icvt[1] = 'o';
			ocvt[2] = 'o';
			break;
		case 's':
			if(*++argv[1] == '\0') ++argv,--argc;
			if(sscanf(argv[1],"%d",&start) != 1) {
				fprintf(stderr,"-s!%%d\n");exit(1);
			}
			goto nexta;
		case 'S':
			csflg = 1;
			goto nexta;
		case 't':
			if(*++argv[1] == '\0') {++argv;--argc;}
			if(mcode)
				tcode = argv[1];
			else
				gettype(argv[1]);
			goto nexta;
		case 'v':
			vflg = Vflg = 1;	/* verbose */
			break;
		case 'w':
			wflg++;
			break;
		case 'X':
		case 'x':
			icvt[1] = 'x';
			ocvt[2] = 'x';
			break;
		default:
			fprintf(stderr,"unknown option (%c), Goodbye\n", c);
			exit(1);
		}
	    }
nexta:
		++argv;
		--argc;
	}

#ifdef PLAN9
	dp = dial(PROMFILE, 0, 0, 0);
#else
	dp = ipcopen(PROMFILE, 2);
#endif
	if (dp < 0) {
		fprintf(stderr, "cannot open promfile %s: %s\n", PROMFILE, errstr);
		exit(1);
	}
#ifndef PLAN9
	setuptty(dp);
#endif
	if((fpr = fdopen(dp, "r")) == (FILE *) NULL) {
		fprintf(stderr, "cannot fdopen, fpr");
		exit(1);
	}
	if((fpw = fdopen(dp, "w")) == (FILE *) NULL) {
		fprintf(stderr, "cannot fdopen, fpw");
		exit(1);
	}
	if((ftty = fopen(TTYNAME, "r")) == (FILE *) NULL) {
		fprintf(stderr, "cannot fopen, %s", TTYNAME);
		exit(1);
	}
	fprintf(fpw, "%c", 0x1b);  /* esc */
	cmd("H\r", 0, "getting prompt",3);	/* ask for prompt */
	cmd("H\r", 0, "getting prompt",3);	/* ask for prompt */
	cmd("%2.2d=\r", 0, "turn off time out",3);
	cmd("%3.3X2A]\r", 0x068, "set programming options",3);
	cmd("%3.3XA\r", xfmt, "setting format",3);
/*	cmd("%4.4X;\r", 0, "setting block size",3);
*/	cmd("%4.4X:\r", 0, "setting device addr",3);
	if(mcode) {
		cmd("%s33]\r", mcode, "setting manufacturer code",3);
		if(!tcode) {
			fprintf(stderr, "mfg code = %s -- no type\n", mcode);
			exit(1);
		}
		cmd("%s34]\r", tcode, "setting part number",3);
	}	
	else if(codeset) {
		if(code < 0x10000)
			cmd("%4.4X@\r", code, "setting family&pinout",3);
		else
			cmd("%6.6X@\r", code, "setting family&pinout",3);
	}
	if(inhib)
		if(strchr(inhib, 'C'))
			cmd("402B]\r", 0, "disable continuity check",2);
	fprintf(fpw,"R\r"); fflush(fpw); /* Respond */
	if((fsn = fscanf(fpr,"%x/%d/%x>",&maxa,&width,&v)) != 3) {
		fprintf(stderr,
	"cannot read response, got: %d returned, maxa=%d, width=%d, v=%d\n",
							fsn, maxa, width, v);
		exit(1);
	}
	if(csflg) {
		cmd("%4.4X<\r", 0, "setting addr of ram",3);
/***		cmd("%4.4X;\r", idat, "setting block size",3);
		cmd("%4.4X:\r", sdev, "setting device start address",3);
***/
		cmd("L\r", 0, "DATAI/O says load rom to ram",2);
		cmd("L\r", 0, "DATAI/O says load rom to ram",2);
	}
	if(nflg) {
		fprintf(stderr, "family&pinout = %x\n",code);
		fprintf(fpw,"[\r"); fflush(fpw);
		if(fscanf(fpr,"%x>",&junk) != 1)
			fprintf(stderr,"cannot read back family&pinout\n");
		else
			fprintf(stderr, "DATAI/O echoed %4.4X\n",junk);
		fprintf(stderr, "maxaddr = %d, width = %d, unprog'd state = %d\n",
							maxa, width, v);
		fprintf(fpw,"G\r"); fflush(fpw);
		if(fscanf(fpr,"%x>",&junk) != 1)
			fprintf(stderr,"cannot read software revision number\n");
		else
			fprintf(stderr, "soft.rev.lev = %4.4X\n",junk);
		if (bflg)
			cmd("B\r", 0, "blank check",1);
		if (!wflg) exit(0);
	}
	bdmsk = ~((1<<width)-1);
	inv = iflg ? (1 << width) - 1 : 0;
	if(maxa >= SIZE) {
		fprintf(stderr,"recompile with SIZE = %d",maxa+1);
		exit(1);
	}
	else if((maxa - start + 1) < 1) {
		fprintf(stderr,"start (%d) exceeds device end (%d)\n",start,maxa);
	}
   if(wflg) {
	for(n=0; n<SIZE; n++) data[n] = 0;
	if(Bflg) {
		if((ndata = read(0, data, SIZE)) <= 0 ) {
			fprintf(stderr, "can't read input file\n");
			exit(1);
		}
		if(ndata == SIZE) {
			fprintf(stderr, "SIZE too small\n");
			exit(1);
		}
		xfmt = 0x50;
		if(inv) {
			for(n = 0; n < ndata; ++n)
				data[n] ^= inv;
		}
		dap = &data[ndata + 1];
	}
	else for(dap = data; ; dap++) {
		if((r = scanf(icvt, dap)) != 1) {
			if(r == EOF)
				break;
			fprintf(stderr,"input not readable with %s\n",icvt);
			exit(1);
		}
		if(bdmsk & *dap) {
			fprintf(stderr, "%x %x %x\n", bdmsk, *dap, bdmsk & *dap);
			fprintf(stderr,"item %d: data wider than device\n",
					dap + 1 - data);
			exit(1);
		}
		*dap ^= inv;
	}
	dapmax = dap;
	ndata = dapmax - data;
	dwrite(start, data, ndata);
	exit(0);
    } else {
	/* read */
	ndata = maxa - start +1;
	if(Vflg)
		fprintf(stderr, "ndata = %d\n", ndata);
	if((err = Dread(start, data, ndata)) < 0) {
		if(Vflg)
			fprintf(stderr,
				"err = %d; retrying with small blocks\n", -err);
		err = retrys(start, data, ndata);
	}
	if(Vflg)
		fprintf(stderr,"outputing\n");
	if(Bflg){
		unsigned char *up = (unsigned char *)data; int i;
		for(i=0; i<err; i++)
			*up++ = data[i];
		write(1, data, err);
	}else
		for(dap = data, dapmax = &data[err]; dap < dapmax; dap++) {
			printf(ocvt,*dap^inv);
		}
	if(Vflg)
		fprintf(stderr,"\n");
	exit(0);
    }
}

dwrite(sdev, sdat, ndat)
unsigned *sdat;
{
	register unsigned *dap, sum;
	if (bflg) {
		cmd("B\r", 0, "blank check", 1);
	}
	sum = 0;
	if(xfmt == 0x50) {
		cmd("%4.4X<\r", 0, "setting addr of ram", 3);
		cmd("%4.4X;\r", ndat, "setting device size", 3);
		cmd("%4.4X:\r", sdev, "setting device start address", 3);
		fprintf(fpw,"I\r"), fflush(fpw);	/*send input command*/
		fprintf(fpw,"%c$A0000,\r",STX);	/*send input command*/
		sum = 0;
		for(dap = sdat; dap < sdat+ndat; dap++) {
			fprintf(fpw,"%2.2X ", *dap);
			sum += *dap;
		}
		fprintf(fpw,"%c", ETX);
		cmd("$S%4.4X,", sum & 0xFFFF, "sum check",1);
	}
	else if(xfmt == 0x91 || xfmt == 0x92) {		/* jedec */
		cmd("%4.4X<\r", 0, "setting addr of ram", 3);
		cmd("%4.4X;\r", 0, "setting device size", 3);
		cmd("%4.4X:\r", sdev, "setting device start address", 3);
		fprintf(fpw,"I\r"); fflush(fpw);
		for(dap = sdat; dap < sdat+ndat; dap++) {
			switch(*dap) {
			case STX:
				sum = 0;
				break;
			case '\n':
				sum += '\r';
			}
			sum += *dap;
			fputc(*dap, fpw);
			if(*dap == ETX) break;
		}
		cmd("%4.4X\r", sum & 0xFFFF, "sum check",1);
	}
	else {
		cmd("%4.4X<\r", 0, "setting addr of ram", 3);
		cmd("%4.4X;\r", 0, "setting device size", 3);
		cmd("%4.4X:\r", sdev, "setting device start address", 3);
		if(Vflg) 
			fprintf(stderr, "loading\n");
		fprintf(fpw,"I\r"); fflush(fpw);
		for(dap = sdat; dap < sdat+ndat; dap++) {
			fputc(*dap, fpw);
		}
		if(Vflg) 
			fprintf(stderr, "sum check\n");
		aprmt("sum check");
	}
	if(!(strchr(inhib, 'C'))) {
		vflg = Vflg;
		sum = 1;
		fprintf(fpw, "DC]\r");
		fflush(fpw);
		while(aprmt("")) {
			if(sum) fprintf(stderr, "insert part\n");
			sum = 0;
			fprintf(fpw, "DC]\r");
			fflush(fpw);
		}
		if(sum == 0) sleep(3);
	}
nextcopy:
	if(Tflg)
		cmd("T\r", 0, "bit test",1);
	if(!nflg) cmd("P\r", 0, "programing",2);
	cmd("V\r", 0, "second verify",1);
	if(--ntimes > 0) {
		fprintf(stderr, "?");
		if(fgetc(ftty) != 'q')
			goto nextcopy;
	}
	return 0;
}

Dread(sdev, sdat, ndat)
unsigned *sdat;
{
	int idat, err, nret = 0;
	while (ndat) {
		idat = min(ndat, 0x4000);
		cmd("%4.4X<\r", 0, "setting addr of ram",3);
		cmd("%4.4X;\r", idat, "setting block size",3);
		cmd("%4.4X:\r", sdev, "setting device start address",3);
		cmd("L\r", 0, "load rom to ram",2);
		if ((err = dread(sdat, idat)) <= 0)
			return err;
		if(xfmt == 0x091)	/* jedec */
			return err;
		sdev += err;
		sdat += err;
		ndat -= idat;
		nret += err;
	}
	return nret;
}

void
gettype(s)
char *s;
{
	struct trans *pt;
	int matches = 0;

	if(codeset) {
		fprintf(stderr,"code already set, Goodbye\n");
		exit(1);
	}
	for(pt = tab; pt->type[0] != '\0'; pt++) {
		if(equstr(pt->type, s)) {
				code = pt->code;
				Tflg = pt->Tflg;
#ifdef PLAN9
				fprintf(stderr, "%s (%s) - %x\n", pt->type, pt->maker, pt->code);
#endif
				matches++;
			}
	}
	if(matches > 1) {
		fprintf(stderr, "ambiguous name %s\n", s);
		exit(1);
	}
	if(matches == 0) {
#ifdef PLAN9
		fprintf(stderr, "unknown type %s\n", s);
		exit(1);
#else
		fprintf(stderr,"unknown type %s\nknown types are:\n", s);
		for(pt = tab; pt->type[0] != '\0'; ) {
			fprintf(stderr,"%s", pt->type);
			if((++pt - tab)%8 == 0)
				fprintf(stderr,"\n");
			else
				fprintf(stderr,"\t");
		}
		fprintf(stderr,"\n");
		exit(1);
#endif
	}
	codeset++;
}

retrys(sdev, sdat, ndat)
unsigned *sdat;
{
	int nndat, i, j, err;
	int count;
	count = 0;
	err = 0;
	for(i = 0; i < ndat; i += SMALL) {
		nndat = ndat - i;
		if(nndat > SMALL) nndat = SMALL;
		for(j = 0; j < 5; j++) {
			if((err = Dread(sdev + i, sdat + i, nndat)) >= 0)
				break;
			if(Vflg)
				fprintf(stderr,"%d",-err);
		}
		if(j >= 5) {
			fprintf(stderr," 5 retrys, drom gives up\n");
			exit(1);
		}
		count += err;
	}
	return(count);
}

dread(sdat, ndat)
int ndat;
unsigned int *sdat;
{
	int ret = 0;

	fprintf(fpw,"O\r"); fflush(fpw); /*output*/
	dap = sdat;
	sum = 0;
	if((xfmt == 0x091) || (xfmt == 0x92)) {
		while(1) {
			switch(c = fgetc(fpr)) {
			case STX:
				sum = 0;
				dap = sdat;
				break;
			case '\n':
				sum += '\r'-'\n';
				break;
			case 'L':
				if(Vflg)
					fprintf(stderr,".");
				break;
				
			}
			if(dap < &data[SIZE-1]){
				sum += c;
				*dap++ = c;
			}
			else 
				goto rout;
			if(c == ETX) {
				if(fscanf(fpr,"%x,",&osum) == 1) {
					if(aprmt("no prompt"))
						exit(1);
					ndat = dap-sdat;
					goto rout;
					}
				fprintf(stderr,"couldn't read D/IO sum\n");
					exit(1);
			}
		}
	}
	else if(xfmt != 0x50) {
		while(1) {
			switch(c = fgetc(fpr)) {
			case '\n':
				if(Vflg)
					fprintf(stderr,".");
				break;
			case '>':
				ndat = dap-sdat;
				goto rout;
			}
			if(dap < &data[SIZE-1])
				*dap++ = c;
			else 
				goto rout;
		}
	}
			
	while(1) {
		if(fscanf(fpr,"%x",dap) != 1) {
			switch(c = getc(fpr)) {
			case '>':
				fprintf(stderr,"unexpected >\n");
				exit(1);
			case '$':
				switch(c = getc(fpr)) {
				default:
					if(Vflg)
						fprintf(stderr,"sec = %c %o\n",
									c,c);
					continue;
				case 'S':
					if(fscanf(fpr,"%x,",&osum) == 1) {
						if(aprmt("no prompt"))
							exit(1);
						goto rout;
					}
					fprintf(stderr,"couldn't read D/IO sum\n");
						exit(1);
				case 'A':
					if(fscanf(fpr,"%x,",&junk) != 1) {
						fprintf(stderr,"$A!%%x,\n");
						exit(1);
					}
					if(junk - (dap-sdat)) {
						ret |= BADADD;
						if(Vflg)
							fprintf(stderr,"1");
#ifndef PLAN9
						sendbreak(dp);
#else
	fprintf(stderr, "attempt to send break\n");
#endif
						while((c = getc(fpr)) != '>');
						goto rout;
					} else if(Vflg)
							fprintf(stderr,".");
					continue;
				}
			case STX:
			case ETX:
				continue;
			default:
				if(Vflg)
					fprintf(stderr,"first = %c %o\n",c,c);
				continue;
			}
		}
		if(dap < &data[SIZE-1])
			sum += *dap++;
		else 
			goto rout;
	}
rout:
	if(ret)
		return(-ret);
	dapmax = dap;
	if(ndat != (dapmax-sdat))
		return(-BADCNT);
	if((sum & 0xFFFF) != (osum & 0xFFFF))
		return(-BADSUM);
	return(ndat);
}

aprmt(emsg)
char *emsg;
{
	int c;
	fflush(fpw);
	while(((c = getc(fpr)) == '\n') || (c == 0) || (c == '\r'));
	if(vflg) fprintf(stderr, "aprmt: %c 0x%x\n", c, c);
	switch(c) {

	case EOF:
		fprintf(stderr,"EOF on %s\n", PROMFILE);
		exit(1);
	case '>':
		if((c = getc(fpr)) == '\n')
			return(0);
		else
		if(c == '\r')
			return(0);
		fprintf(stderr, "after > %c\n", c);
		/* fall into break */
	case 'F':
		if(vflg)
			fprintf(stderr,"%s failed\n",emsg);
		break;
	default:
		fprintf(stderr,"bad char was %c (0%o)\n",c,c);
		exit(1);
	}
	if(vflg) {
		fprintf(stderr,"error codes ");
		fprintf(fpw,"F\r"); fflush(fpw);
		while((c = getc(fpr)) != '>') fputc(c, stderr);
		fprintf(stderr, " ");
		fprintf(fpw,"X\r"); fflush(fpw);
		while((c = getc(fpr)) != '>') fputc(c, stderr);
		fprintf(stderr, "\n");
		fflush(stderr);
	}
	return(1);
}

equstr(s1, s2)
	char	*s1, *s2;
{
#ifdef PLAN9
	return (strstr(s1, s2));
#else
	while(*s1++ == *s2) {
		if(*s2++==0) {
			return(1);
		}
	}
	return(0);
#endif
}

void
cmd(s0,p,s1,ntry)
char *s0, *s1;
{
	int try;
	char buffer[256];
	if(vflg) {
		sprintf(buffer, s0, p);
		fprintf(stderr, "%s: %s (%d times)\n", s1, buffer, ntry);
	}
	for (try=0; try<ntry; try++) {
		fprintf(fpw, s0, p);
		if(!aprmt(s1)) return;
		if((try > 0) && vflg)
			fprintf(stderr,"retrying %s\n", s1);
	}
	fprintf(stderr,"urom gives up\n");
	exit(1);
	return;		/* to shut compiler up */
}

/*#ifdef 3B
#include <termio.h>
struct termio vec;

setuptty(fd)
	int fd;
{
	struct sgttyb vvec;
	struct ttydevb dvec;
	extern int tty_ld;

	if(ioctl(fd, FIOLOOKLD, 0) != tty_ld){
		if(ioctl(fd, FIOPUSHLD, &tty_ld) < 0){
			perror("tty_ld");
			exit(1);
		}
	}
	vvec.sg_flags = CRMOD|TANDEM;
	vvec.sg_erase = 0;
	vvec.sg_kill = 0;
	ioctl(fd, TIOCGDEV, &dvec);
	if(ioctl(fd, TIOCSETP, &vvec) < 0){
		perror("ioctl");
		exit(1);
	}
	dvec.ispeed = dvec.ospeed = B9600;
	dvec.flags = F8BIT;
	if(ioctl(fd, TIOCSDEV, &dvec) < 0){
		perror("ioctl1");
		exit(1);
	}
}

sendbreak(fd)
	int fd;
{
	return ioctl(fd, TCSBRK, 0);
}

#else
*/
#ifndef PLAN9
struct sgttyb vec;
struct ttydevb dvec;
extern int tty_ld;

setuptty(fd)
	int fd;
{
	if(ioctl(fd, FIOLOOKLD, 0) != tty_ld){
		if(ioctl(fd, FIOPUSHLD, &tty_ld) < 0){
			perror("bad tty_ld");
			exit(1);
		}
	}
	vec.sg_flags = ANYP|TANDEM|CRMOD;
	vec.sg_erase = '\b';
	vec.sg_kill = '@';
	if(ioctl(fd, TIOCSETP, &vec) < 0){
		perror("ioctl: setp");
		exit(1);
	}
	dvec.ispeed = dvec.ospeed = B9600;
	dvec.flags = F8BIT|EVENP|ODDP;
	if(ioctl(fd, TIOCSDEV, &dvec) < 0){
		perror("ioctl: sdev");
		exit(1);
	}
}

sendbreak(fd)
	int fd;
{
	return ioctl(fd, TIOCSBRK, 0);
}
#endif
/*#endif 3B
*/
#ifdef PLAN9
void
readcodes(stream)
FILE *stream;
{
	int rec, n, flags = 0, note;
	int code[2];
	char name[SMALL];
	char manufacturer[SMALL];
	for (rec=0; rec<MAXCODES; rec++)
	{
		n = fscanf(stream, "%[-+/*A-Za-z0-9 ] %[-+/*A-Za-z0-9 ] %x %x %d\n", manufacturer, name, &code[0], &code[1], &note);
		if (n == EOF) break;
		if (n < 4){
			fprintf(stderr, "line too short (%d) in code file %s at line %d\n",
			n, CODEFILE, rec+1);
			if(n > 0)
				fprintf(stderr, "manufacturer %s\n", manufacturer);
			if(n > 1)
				fprintf(stderr, "name %s\n", name);
			if(n > 2)
				fprintf(stderr, "code[0] %4.4ux\n", code[0]);
			if(n > 3)
				fprintf(stderr, "code[1] %4.4ux\n", code[1]);
		}else
		if (strlen(name) > MAXNAME)
			fprintf(stderr, "name too long in code file %s at line %d\n",
			CODEFILE, rec+1);
		else
		{
			strncpy(tab[rec].type, name, MAXNAME);
			strncpy(tab[rec].maker, manufacturer, MAXNAME);
			tab[rec].Tflg = flags;
			tab[rec].code = (code[0] << 12) | (code[1] & 0xfff);
		} /* end else */
	} /* end for */
} /* end readcodes */
#else
void
readcodes(stream)
FILE *stream;
{
	int rec, n, flags, code;
	char name[SMALL];
	for (rec=0; rec<MAXCODES; rec++)
	{
		n = fscanf(stream, "%s %x %x\n", name, &flags, &code);
		if (n == EOF) break;
		if ((n < 3) || (strlen(name) > 12))
			fprintf(stderr, "format error in code file %s at line %d\n",
			CODEFILE, rec+1);
		else
		{
			strncpy(tab[rec].type, name, 12);
			tab[rec].Tflg = flags;
			tab[rec].code = code;
		} /* end else */
	} /* end for */
} /* end readcodes */
#endif
