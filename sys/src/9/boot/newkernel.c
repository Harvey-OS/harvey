#include <u.h>
#include <libc.h>
#include <../boot/boot.h>

#define CMPSIZE 128
void
newkernel(void)
{
	char *p;
	char file[2*NAMELEN];
	char partition[2*NAMELEN];
	char prompt[8];
	char boot[CMPSIZE], installed[CMPSIZE];
	long x, i;
	int in, out;
	Dir d;

	/* get the boot kernel's first CMPSIZE bytes */
	sprint(partition, "%sboot", bootdisk);
	out = open(partition, ORDWR);
	if(out < 0)
		return;
	if(read(out, boot, CMPSIZE) != CMPSIZE || seek(out, 0, 0) < 0){
		close(out);
		return;
	}

	sprint(file, "/%s/9%s", cputype, conffile);
	in = open(file, OREAD);
	if(in < 0){
		close(out);
		return;
	}
	if(dirfstat(in, &d)<0 || read(in, installed, CMPSIZE)!=CMPSIZE || seek(in, 0, 0)<0){
		close(in);
		close(out);
		return;
	}

	/* if they agree in the first CMPSIZE, assume they are the same */
	if(memcmp(installed, boot, CMPSIZE) == 0){
		close(in);
		close(out);
		return;
	}

	strcpy(prompt, "y/n");
	outin("\nThis kernel is out of date.  Copy a new one to disk?",
		prompt, sizeof(prompt));
	if(*prompt != 'y')
		return;

	/* copy all kernel into memory, then to disk */
	x = ((d.length + 511)/512)*512;
	p = sbrk(x);
	if(p == (char*)-1){
		print("sbrk %d failed\n", x);
		return;
	}
	if(read(in, p, d.length) != d.length){
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
			break;
		}
		p += i;
	}
	close(in);
	close(out);
}
