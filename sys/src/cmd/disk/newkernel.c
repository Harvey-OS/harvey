#include <u.h>
#include <libc.h>

void
error(char* msg)
{
	fprint(2, "newkernel: %s\n", msg);
	exits(msg);
}

int
outin(char *prompt, char *def, int len)
{
	int n;
	char buf[256];

	do{
		print("%s[%s]: ", prompt, def);
		n = read(0, buf, len);
	}while(n==0);
	if(n < 0)
		error("can't read stdin; please reboot");
	if(n != 1){
		buf[n-1] = 0;
		strcpy(def, buf);
	}
	return n;
}

void
newkernel(char *file)
{
	Dir this;
	Dir installed;
	char *partition;
	char prompt[8];
	long x, i;
	int in, out;
	char *p;

	/* this kernel's link time */
	if(dirstat("/boot", &this) < 0){
		print("can't stat /boot\n");
		return;
	}

	/* the installed kernel's installation time */
	if(dirstat(file, &installed) < 0){
		print("can't stat %s\n", file);
		return;
	}

	/*  if the current kernel is 10 minutes older than the
	 *  installed kernel, ask user if he wants to update.
	 */
	if(installed.mtime < this.mtime + 10*60)
		return;

	/*
	 *  find a boot partition
	 */
	if(dirstat("/dev/hd0boot", &this) < 0){
		if(dirstat("/dev/hd1boot", &this) < 0)
			return;
		partition = "/dev/hd1boot";
	} else
		partition = "/dev/hd0boot";

	strcpy(prompt, "y/n");
	outin("\nThis kernel is out of date.  Copy a new one to disk?",
		prompt, sizeof(prompt));
	if(*prompt != 'y')
		return;

	/* copy all kernel into memory, then to disk */
	x = ((installed.length + 511)/512)*512;
	p = sbrk(x);
	if(p == (char*)-1){
		print("sbrk %d failed\n", x);
		return;
	}
	in = open(file, OREAD);
	if(in < 0){
		print("can't open %s\n", file);
		return;
	}
	out = open(partition, OWRITE);
	if(out < 0){
		print("can't open %s\n", partition);
		close(in);
		return;
	}
	if(read(in, p, installed.length) != installed.length){
		print("couldn't read %s\n", file);
		close(in);
		close(out);
		return;
	}
	for(; x > 0; x -= i){
		i = x > 4*1024 ? 4*1024 : x;
		if(write(out, p, i) != i){
			print("error writing %s, you may be in deep trouble!\n",
				partition);
			return;
		}
		p += i;
	}
	close(in);
	close(out);

	for(;;){
		print("new kernel loaded, please reboot\n");
		sleep(2);
	}
}

void
main(int argc, char **argv)
{
	if(argc > 1)
		newkernel(argv[1]);
	exits(0);
}
