#include <u.h>
#include <libc.h>
#include <regexp.h>
#include <bio.h>
#include "debug.h"
#include "tap.h"
#include "chain.h"
#include "jtag.h"
#include "icert.h"
#include "mmu.h"
#include "mpsse.h"

static Biobuf bin;

static void
testsh(JMedium *jmed)
{
	char *ln;
	while(ln = Brdstr(&bin, '\n', 1)){
		if(ln == nil)
			break;
		fprint(2, "[%s]\n", ln);
		if(strcmp(ln, "") == 0 || ln[0] == '#'){
			free(ln);
			continue;
		}
		if(pushcmd(jmed->mdata, ln) < 0)
			fprint(2, "error: %r\n");
		free(ln);
	}
}

static void
hwreset(JMedium *jmed)
{
	/* BUG make this medium independant... */
	if(pushcmd(jmed->mdata, "TmsCsOut EdgeDown LSB B0x7 0x7f") < 0) 
		sysfatal("resetting %r");
	if(jmed->flush(jmed->mdata))
		sysfatal("resetting %r");
	sleep(1000);
	jmed->resets(jmed->mdata, 1, 0);
	sleep(200);
	jmed->resets(jmed->mdata, 1, 1);
	sleep(200);
	jmed->resets(jmed->mdata, 0, 1);
	sleep(200);
	jmed->resets(jmed->mdata, 0, 0);
	sleep(200);
}

static void
usage(void)
{
	fprint(2, "Usage: %s [-t] [-r] [-d 'a'...] jtagfile\n", argv0);
	exits("usage");
}

static Chain ch;
static EiceChain1 ec1;
static EiceChain2 ec2;
static char dbgstr[256];

static ArmCtxt ctxt;
static u32int regs[16];

static void
testice(JMedium *jmed)
{
	int res, i;
	u32int cpuid, data;
	debug[Dctxt] = 1;

	cpuid = armidentify(jmed);
	fprint(2, "---- Cpuid --- %8.8ux\n", cpuid);
	if(cpuid == ~0)
		sysfatal("not feroceon or WFI bug...");

	fprint(2, "---- Bypass probe --- \n");
	if(armbpasstest(jmed) < 0)
		sysfatal("bypass test");

	fprint(2, "---- Check state --- \n");
	res = setchain(jmed, ChCommit, 2);
	if(res < 0)
		sysfatal("setchain %r");

	icegetreg(jmed, DebStsReg);

	fprint(2, "---- Freeze  --- \n");
	res = iceenterdebug(jmed, &ctxt);
	if(res < 0)
		sysfatal("could not enter debug");
	fprint(2, "\n\n\tIn debug state for 1 sec\n\n");
	sleep(1000);
	fprint(2, "---- MMU test  --- \n");
	res = mmurdregs(jmed, &ctxt);
	if(res < 0)
		sysfatal("mmurdregs %r");

	printmmuregs(&ctxt, dbgstr, sizeof dbgstr);
	fprint(2, "MMU state:\n%s\n", dbgstr);

	fprint(2, "---- Read mem  --- \n");
	for(i = 0; i < 10; i++){
		res = armrdmemwd(jmed, ctxt.r[15]+4*i, &data, 4);
		if(res < 0){
			fprint(2, "Error reading %#8.8ux pc[%d]\n", ctxt.r[15]+4*i, i);
			break;
		}
		fprint(2, "Read data %#8.8ux addr %#8.8ux pc[%d]\n",
				data, ctxt.r[15]+4*i, i);
	}
	
	fprint(2, "---- Write mem (dangerous test) --- \n");

	if(0)
		for(i = 0; i < 10; i++){
			data = 0;
			fprint(2, "Write data %#8.8ux addr %#8.8ux pc[%d]\n", data, ctxt.r[15]+4*i, i);
			res = armwrmemwd(jmed, ctxt.r[15]+4*i, data, 4);
			if(res < 0){
				fprint(2, "Error reading %#8.8ux pc[%d]\n", ctxt.r[15]+4*i, i);
				break;
			}
		}
	
	fprint(2, "---- Unfreeze  --- \n");
	res = iceexitdebug(jmed, &ctxt);
	if(res < 0)
		sysfatal("could not exit debug");
}


void
main(int argc, char *argv[])
{
	JMedium *jmed;
	int tsh, rsh, i, jtagfd;
	char *deb;
	
	deb = nil;
	argv0 = "mpssetest";
	tsh = rsh = 0;

	ARGBEGIN{
	case 't':
		tsh = 1;
		break;
	case 'r':
		rsh = 1;
		break;
	case 'd':
		deb = EARGF(usage());
		break;
	default:
		usage();
	} ARGEND
	
	if(argc != 1)
		usage();

	jtagfd = open(argv[0], ORDWR);
	if(jtagfd < 0)
		sysfatal("cannot open jtag file"); 

	if(deb  != nil)
		for(i = 0; i < strlen(deb); i++)
			debug[deb[i]]++;
	Binit(&bin, 0, OREAD);

	if(tsh){
		 jmed = newmpsse(jtagfd, Sheeva);
		if(jmed == nil)
			sysfatal("newmpsse %r");
		testsh(jmed);
	}
	else if(rsh){
		jmed = initmpsse(jtagfd, Sheeva);
		if(jmed == nil)
			sysfatal("initialization %r");
		hwreset(jmed);
	}
	else {
		jmed = initmpsse(jtagfd, Sheeva);
		if(jmed == nil)
			sysfatal("initialization %r");
	
		testice(jmed);
	}
	Bterm(&bin);
	jmed->term(jmed);
}
