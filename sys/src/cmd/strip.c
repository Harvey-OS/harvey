#include <u.h>
#include <libc.h>
#include <a.out.h>

int	strip(char*);

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
	int fd;
	Exec exec;
	char *data;
	Dir d;
	long n, len;
	int i, j;

	/*
	 *  make sure file is executable
	 */
	if(dirstat(file, &d) < 0){
		perror(file);
		return 1;
	}
	if((d.qid.path & (CHDIR|CHAPPEND|CHEXCL))){
		fprint(2, "strip: %s must be executable\n", file);
		return 1;
	}
	/*
	 *  read it's header and see if that makes sense
	 */
	fd = open(file, OREAD);
	if(fd < 0){
		perror(file);
		return 1;
	}
	n = read(fd, &exec, sizeof exec);
	if (n != sizeof(exec)) {
		fprint(2, "strip: Unable to read header of %s\n", file);
		close(fd);
		return 1;
	}
	i = ben(exec.magic);
	for (j = 8; j < 24; j++)
		if (i == _MAGIC(j))
			break;
	if (j >= 24) {
		fprint(2, "strip: %s is not a recognizable binary\n", file);
		close(fd);
		return 1;
	}

	len = ben(exec.data) + ben(exec.text);
	if(len+sizeof(exec) == d.length) {
		fprint(2, "strip: %s is already stripped\n", file);
		close(fd);
		return 0;
	}
	if(len+sizeof(exec) > d.length) {
		fprint(2, "strip: %s has strange length\n", file);
		close(fd);
		return 1;
	}
	/*
	 *  allocate a huge buffer, copy the header into it, then
	 *  read the file.
	 */
	data = malloc(len+sizeof(exec));
	if (!data) {
		fprint(2, "strip: Malloc failure. %s too big to strip.\n", file);
		close(fd);
		return 1;
	}
	/*
	 *  copy exec, text and data
	 */
	exec.syms = 0;
	exec.spsz = 0;
	exec.pcsz = 0;
	memcpy(data, &exec, sizeof(exec));
	n = read(fd, data+sizeof(exec), len);
	if (n != len) {
		perror(file);
		close(fd);
		return 1;
	}
	close(fd);
	if(remove(file) < 0) {
		perror(file);
		free(data);
		return 1;
	}
	fd = create(file, OWRITE, d.mode);
	if (fd < 0) {
		perror(file);
		free(data);
		return 1;
	}
	n = write(fd, data, len+sizeof(exec));
	if (n != len+sizeof(exec)) {
		perror(file);
		close(fd);
		free(data);
		return 1;
	} 
	close(fd);
	free(data);
	return 0;
}
