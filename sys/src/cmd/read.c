#include <u.h>
#include <libc.h>
#include <bio.h>

void
main(int argc, char *argv[])
{
	char c;
	int n, fd;
	Biobuf out;

	if(argc > 1){
		fd = open(argv[1], OREAD);
		if(fd < 0){
			perror(argv[1]);
			exits("open");
		}
	}else
		fd = 0;
	Binit(&out, 1, OWRITE);
	for(;;){
		n = read(fd, &c, 1);
		if(n < 0){
			perror("read");
			exits("read");
		}
		if(n == 0)
			exits("eof");
		Bputc(&out, c);
		if(c == '\n')
			break;
	}
	exits(0);
}
