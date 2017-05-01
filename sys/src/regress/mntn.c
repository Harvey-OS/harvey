#include <u.h>
#include <libc.h>

const char *contents = "This is a string of bytes in a file";
const char *changedcontents = "This is a xxxxxx of bytes in a file";
const char *changedcontents2 = "This is a yyyyyy of bytes in a file";

void
failonbad(int rc, const char* reason)
{
	if(rc < 0){
		fprint(2,"FAIL %r - %s\n", reason);
		exits("FAIL");
	}
}

void
writetest()
{
	int rc;
	char buffer[1000];
	int fd = create("/n/ram/stuff", ORDWR, 0666);
	failonbad(fd, "File open in ramdisk");
	rc = write(fd, contents, strlen(contents));
	if(rc != strlen(contents)){
		failonbad(-1, "Incorrect length written");
	}

	// Check contents
	rc = seek(fd, 0, 0);
	failonbad(rc, "seek failed");
	rc = read(fd, buffer, strlen(contents));
	if(rc != strlen(contents)){
		failonbad(-1, "Incorrect length read");
	}
	if(strncmp(buffer, contents, strlen(contents))){
		failonbad(-1, "compare failed");
	}

	// update the string and verify it
	rc = seek(fd, 10, 0);
	failonbad(rc, "seek failed");
	rc = write(fd, "xxxxxx", 6);
	if (rc != 6){
		failonbad(-1, "Incorrect update length written");
	}
	rc = seek(fd, 0, 0);
	failonbad(rc, "seek failed");
	rc = read(fd, buffer, strlen(contents));
	if(rc != strlen(contents)){
		failonbad(-1, "Incorrect update length read");
	}
	if(strncmp(buffer, changedcontents, strlen(contents))){
		failonbad(-1, "changed compare failed");
	}

	rc = seek(fd, 10, 0);
	failonbad(rc, "seek failed");
	write(fd, "yyy", 3);
	write(fd, "yyy", 3);
	rc = seek(fd, 0, 0);
	failonbad(rc, "seek failed");
	rc = read(fd, buffer, strlen(contents));
	if(rc != strlen(contents)){
		failonbad(-1, "Incorrect update length read");
	}
	if(strncmp(buffer, changedcontents2, strlen(contents))){
		failonbad(-1, "changed compare failed");
	}

	rc = close(fd);
	failonbad(rc, "File close in ramdisk");
}

void
main(int argc, char *argv[])
{
	int rc, fd;
	Dir* opstat;

	switch(fork()){
	case -1:
		fprint(2, "fork fail\n");
		exits("FAIL");
	case 0:
		execl("/bin/ramfs", "ramfs", "-S", "ramfs", nil);
		fprint(2, "execl fail\n");
		exits("execl");
	default:
		do{
			sleep(10);
			opstat = dirstat("/srv/ramfs");
		}while (opstat == nil);
		free(opstat);
		fd = open("/srv/ramfs", ORDWR);
		rc = mount(fd, -1, "/n/ram", MREPL|MCREATE, "", (int)'N');
		if(rc < 0){
			failonbad(-1, "mount failed");
			exits("FAIL");
		}
		writetest();
		break;
	}
	print("PASS\n");
	exits("PASS");
}
