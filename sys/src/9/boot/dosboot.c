#include <u.h>
#include <libc.h>
#include "../boot/boot.h"

int	printcol;

static void	dossrv(void);
static void	swapproc(void);

void
dosboot(void)
{
	int fd;

	open("#c/cons", OREAD);
	open("#c/cons", OWRITE);
	open("#c/cons", OWRITE);

	/*
	 *  start to dos file system server
	 */
	dossrv();
	sleep(1000);
	fd = open("#s/boot", ORDWR);
	if(fd < 0)
		fatal("open #s/boot");

	/*
	 *  pick a floppy and mount it as root
	 */
	if(bind("/", "/", MREPL) < 0)
		fatal("bind /");
	if(mount(fd, "/", MAFTER|MCREATE, "#f/fd0disk") < 0)
		if(mount(fd, "/", MAFTER|MCREATE, "#f/fd1disk") < 0)
			if(mount(fd, "/", MAFTER|MCREATE, "#S/sdC0/dos") < 0)
				fatal("mount /");
	close(fd);

	settime(1);
	swapproc();

	execl("/386/init", "init", "-mt", 0);
	fatal("/386/init");
}

static void
dossrv(void)
{
	print("dossrv...");
	if(bind("#c", "/dev", MREPL) < 0)
		fatal("bind #c");
	if(bind("#p", "/proc", MREPL) < 0)
		fatal("bind #p");
	switch(fork()){
	case -1:
		fatal("fork");
	case 0:
		execl("/cfs", "cfs", "boot", 0);
		fatal("can't exec cfs");
	default:
		break;
	}
}

static void
swapproc(void)
{
	int fd;

	fd = open("#c/swap", OWRITE);
	if(fd < 0){
		warning("opening #c/swap");
		return;
	}
	if(write(fd, "start", 5) <= 0)
		warning("starting swap kproc");
}
