#include <u.h>
#include <libc.h>

#define NUM_FILES 100
#define FILE_SIZE 3000

const char *contents = "Test File";
char* dir;

void
main(int argc, char **argv){
	char filename[1024];
	int i,j;
	Dir *resultFile;

	if (argc < 2){
		print("Please specify  a directory\n");
		print("FAIL\n");
		exits("FAIL");
	}
	dir = argv[1];
	for (i = 0; i < NUM_FILES; i++){
		sprint(filename, "%s/file%d", dir, i);
		int fd = create(filename, OWRITE, 0777);
		if (fd < 0) {
			print("FAIL could not open file %s\n", filename);
			exits("FAIL");
		}
		for(j = 0; j < FILE_SIZE; j++) {
			write(fd, contents, strlen(contents));
		}
		close(fd);
		resultFile = dirstat(filename);
		if (resultFile->length != strlen(contents) * FILE_SIZE){
			print("FAIL file size %s incorrect expected %d actual %d\n", filename, strlen(contents) * FILE_SIZE, resultFile->length);
			exits("FAIL");
		}
		free(resultFile);
	}
	for (i = 0; i < NUM_FILES; i++){
		sprint(filename, "%s/file%d", dir, i);
		if (remove(filename) < 0){
			print("FAIL unable to remove file\n");
			exits("FAIL");
		}
	}

	print("PASS\n");
	exits("PASS");
}
