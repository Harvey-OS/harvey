#include <u.h>
#include <libc.h>
#include <a.out.h>

int	strip(char*);
int	rename(char*, char*);
long	ben(long);

void
main(int argc, char *argv[])
{
	int i;
	int rv;

	rv = 0;
	for(i = 1; i < argc; i++)
		rv |= strip(argv[i]);
	if(rv)
		exits("error");
	exits(0);
}

long
ben(long xen)
{
	union
	{
		long	xen;
		uchar	uch[sizeof(long)];
	} u;

	u.xen = xen;
	return (u.uch[0] << 24) | (u.uch[1] << 16) | (u.uch[2] << 8) | (u.uch[3] << 0);
}

int
strip(char *file)
{
	int in, out;
	Exec exec, bexec;
	char tmp[NAMELEN];
	char buf[4*1024];
	Dir d;
	long n, len;

	/*
	 *  make sure file is executable
	 */
	if(dirstat(file, &d) < 0){
		perror(file);
		return -1;
	}
	if((d.qid.path & (CHDIR|CHAPPEND|CHEXCL))
	|| !(d.mode & (0111))){
		fprint(2, "%s must be executable\n", file);
		return -1;
	}

	/*
	 *  read it's header and see if that makes sense
	 */
	in = open(file, OREAD);
	if(in < 0){
		perror(file);
		return -1;
	}
	n = read(in, &exec, sizeof exec);

	bexec.magic = ben(exec.magic);
	bexec.text = ben(exec.text);
	bexec.data = ben(exec.data);

	if(n != sizeof(exec) || (bexec.magic!=V_MAGIC && bexec.magic!=Z_MAGIC
				&& bexec.magic!=K_MAGIC && bexec.magic!=A_MAGIC
				 && bexec.magic!=I_MAGIC)){
		fprint(2, "%s is not a binary I recognize\n", file);
		close(in);
		return -1;
	}

	len = bexec.data + bexec.text;
	if(len+sizeof(exec) == d.length) {
		fprint(2, "%s is already stripped\n", file);
		close(in);
		return 0;
	}
	if(len+sizeof(exec) > d.length) {
		fprint(2, "%s has strange length\n", file);
		close(in);
		return -1;
	}

	/*
	 *  make a temporary file to build the new binary in
	 */
	strcpy(tmp, "stripXXXXXXXXXXX");
	mktemp(tmp);
	out = create(tmp, OWRITE, d.mode);
	if(out < 0) {
		perror(tmp);
		close(in);
		return -1;
	}

	/*
	 *  copy exec, text and data
	 */
	exec.syms = 0;
	exec.spsz = 0;
	exec.pcsz = 0;
	if(write(out, &exec, sizeof(exec)) != sizeof(exec)) {
		perror(tmp);
		goto rmtmp;
	}
	while(len > 0) {
		n = sizeof(buf);
		if(n > len)
			n = len;
		n = read(in, buf, n);
		if(n <= 0) {
			perror(file);
			goto rmtmp;
		}
		if(write(out, buf, n) != n) {
			perror(tmp);
			goto rmtmp;
		}
		len -= n;
	}
	close(in);
	close(out);
	if(remove(file) < 0) {
		perror(file);
		goto rmtmp;
	}
	if(rename(tmp, file) < 0) {
		fprint(2, "can't rename %s to %s\n", tmp, file);
		return -1;
	}
	return 0;

rmtmp:
	close(in);
	close(out);
	remove(tmp);
	return -1;
}

int
rename(char *from, char *to)
{
	Dir d;
	char *toelem;

	/*
	 * get last element of to path
	 */
	toelem = utfrrune(to, '/');
	if(toelem)
		toelem++;
	else
		toelem = to;

	/*
	 *  change the stat buffer to change the name
	 */
	if(dirstat(from, &d) < 0) {
		perror(from);
		return -1;
	}
	strcpy(d.name, toelem);
	return dirwstat(from, &d);
}
