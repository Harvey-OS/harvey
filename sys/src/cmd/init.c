#include <u.h>
#include <libc.h>
#include <auth.h>

void	readenv(char*, char*, int);
void	setenv(char*, char*);
void	cpenv(char*, char*);
void	closefds(void);
void	fexec(void(*)(void));
void	rcexec(void);
void	cpustart(void);
void	pass(int);

char	*service;
char	*cmd;
char	cpu[NAMELEN];
char	sysname[NAMELEN];
int	manual;
int	iscpu;

void
main(int argc, char *argv[])
{
	char user[NAMELEN];
	int consctl, key;

	closefds();

	service = "cpu";
	manual = 0;
	ARGBEGIN{
	case 'c':
		service = "cpu";
		break;
	case 'm':
		manual = 1;
		break;
	case 't':
		service = "terminal";
		break;
	}ARGEND
	cmd = *argv;

	readenv("#e/cputype", cpu, sizeof cpu);
	setenv("#e/objtype", cpu);
	setenv("#e/service", service);
	cpenv("/adm/timezone/local", "#e/timezone");
	readenv("#c/user", user, sizeof user);
	readenv("#c/sysname", sysname, sizeof sysname);

	newns(user, 0);

	iscpu = strcmp(service, "cpu")==0;

	if(iscpu && manual == 0)
		fexec(cpustart);

	for(;;){
		if(iscpu){
			consctl = open("#c/consctl", OWRITE);
			key = open("#c/key", OREAD);
			if(consctl<0 || key<0 || write(consctl, "rawon", 5) != 5)
				print("init: can't check password; insecure\n");
			else{
				pass(key);
				write(consctl, "rawoff", 6);
			}
			close(consctl);
			close(key);
		}
		print("\ninit: starting /bin/rc\n");
		fexec(rcexec);
		manual = 1;
		cmd = 0;
		sleep(1000);
	}
}

void
pass(int fd)
{
	char key[DESKEYLEN];
	char typed[32];
	char crypted[DESKEYLEN];
	int i;

	for(;;){
		readenv("#c/sysname", sysname, sizeof sysname);
		print("\n%s password:", sysname);
		for(i=0; i<sizeof typed; i++){
			if(read(0, typed+i, 1) != 1){
				print("init: can't read password; insecure\n");
				return;
			}
			if(typed[i] == '\n'){
				typed[i] = 0;
				break;
			}
		}
		if(i == sizeof typed)
			continue;
		if(passtokey(crypted, typed) == 0)
			continue;
		seek(fd, 0, 0);
		if(read(fd, key, DESKEYLEN) != DESKEYLEN){
			print("init: can't read key; insecure\n");
			return;
		}
		if(memcmp(crypted, key, sizeof key))
			continue;
		/* clean up memory */
		memset(crypted, 0, sizeof crypted);
		memset(key, 0, sizeof key);
		return;
	}
}

void
fexec(void (*execfn)(void))
{
	Waitmsg w;
	int pid, i;

	switch(pid=fork()){
	case 0:
		(*execfn)();
		print("init: exec error: %r\n");
		exits("exec");
	case -1:
		print("init: fork error: %r\n");
		exits("fork");
	default:
	casedefault:
		i = wait(&w);
		if(i == -1)
			print("init: wait error: %r\n");
		else if(i != pid)
			goto casedefault;
		if(strcmp(w.msg, "exec") == 0){
			print("init: sleeping because exec failed\n");
			for(;;)
				sleep(1000);
		}
		if(w.msg[0])
			print("init: rc exit status: %s\n", w.msg);
		break;
	}
}

void
rcexec(void)
{
	if(cmd)
		execl("/bin/rc", "rc", "-c", cmd, 0);
	else if(manual || iscpu)
		execl("/bin/rc", "rc", 0);
	else if(strcmp(service, "terminal") == 0)
		execl("/bin/rc", "rc", "-c", ". /rc/bin/termrc; home=/usr/$user; cd; . lib/profile", 0);
	else
		execl("/bin/rc", "rc", 0);
}

void
cpustart(void)
{
	execl("/bin/rc", "rc", "-c", "/rc/bin/cpurc", 0);
}

void
readenv(char *name, char *val, int len)
{
	int f;

	f = open(name, OREAD);
	if(f < 0){
		print("init: can't open %s\n", name);
		return;	
	}
	len = read(f, val, len-1);
	close(f);
	if(len < 0)
		print("init: can't read %s\n", name);
	else
		val[len] = '\0';
}

void
setenv(char *var, char *val)
{
	int fd;

	fd = create(var, OWRITE, 0644);
	if(fd < 0)
		print("init: can't open %s\n", var);
	else{
		fprint(fd, val);
		close(fd);
	}
}

void
cpenv(char *file, char *var)
{
	int i, fd;
	char buf[8192];

	fd = open(file, OREAD);
	if(fd < 0)
		print("init: can't open %s\n", file);
	else{
		i = read(fd, buf, sizeof(buf)-1);
		if(i <= 0)
			print("init: can't read %s: %r\n", file);
		else{
			close(fd);
			buf[i] = 0;
			setenv(var, buf);
		}
	}
}

/*
 *  clean up after /boot
 */
void
closefds(void)
{
	int i;

	for(i = 3; i < 30; i++)
		close(i);
}
