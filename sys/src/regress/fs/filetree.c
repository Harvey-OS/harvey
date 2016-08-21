#include <u.h>
#include <libc.h>

#define NUM_FILES 10
#define NUM_DIRS 10

char* dir;

void
main(int argc, char **argv){
	char dirname[1024];
	char filename[1024];
	int i,k;

	if (argc < 2){
		print("Please specify  a directory\n");
		print("FAIL\n");
		exits("FAIL");
	}
	dir = argv[1];
	for (k = 0; k < NUM_DIRS; k++){
		sprint(dirname, "%s/dir%d", dir, k);
		print("creating dir %s\n", dirname);
		int fd = create(dirname, OREAD, DMDIR | 0777);
		if (fd < 0) {
			print("FAIL could not create dir %s %r\n", filename);
			exits("FAIL");
		}
		for (i = 0; i < NUM_FILES; i++){
			sprint(filename, "%s/dir%d/file%d", dir, k, i);
			print("creating %s\n", filename);
			int fd = create(filename, OWRITE, 0777);
			if (fd < 0) {
				print("FAIL could not open file %s %r\n", filename);
				exits("FAIL");
			}
			close(fd);
		}
	}
	for (k = 0; k < NUM_DIRS; k++){
		sprint(dirname, "%s/dir%d", dir, k);
		for (i = 0; i < NUM_FILES; i++){
			sprint(filename, "%s/dir%d/file%d", dir, k, i);
			print("removing %s\n", filename);
			if (remove(filename) < 0){
				print("FAIL unable to remove file\n");
				exits("FAIL");
			}
		}
		print("removing %s\n", dirname);
		if (remove(dirname) < 0){
			print("FAIL unable to remove directory\n");
			exits("FAIL");
		}
		print("removed %s\n", dirname);
	}

	print("PASS\n");
	exits("PASS");
}
