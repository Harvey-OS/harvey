#include <u.h>
#include <libc.h>

#define NUM_FILES 100

char* dir;

void
main(int argc, char **argv){
	char filename[1024];
	int i;

	if (argc < 2){
		print("Please specify  a directory\n");
		print("FAIL\n");
		exits("FAIL");
	}
	dir = argv[1];
	for (i = 0; i < NUM_FILES; i++){
		sprint(filename, "%s/file%d", dir, i);
		print("creating %s\n", filename);
		int fd = create(filename, OWRITE, 0777);
		if (fd < 0) {
			print("FAIL could not open file %s %r\n", filename);
			exits("FAIL");
		}
		close(fd);
	}
	for (i = 0; i < NUM_FILES; i++){
		sprint(filename, "%s/file%d", dir, i);
		print("removing %s\n", filename);
		if (remove(filename) < 0){
			print("FAIL unable to remove file\n");
			exits("FAIL");
		}
	}

	print("PASS\n");
	exits("PASS");
}
