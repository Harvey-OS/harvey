#include <u.h>
#include <libc.h>
#include <bio.h>
#include "dns.h"
#include "ip.h"

void
main(void)
{
	int fd, n;
	Biobuf in;
	char *line;
	char buf[1024];

	fd = open("/net/dns", ORDWR);
	if(fd < 0){
		print("can't open: %r\n");
		exits(0);
	}
	Binit(&in, 0, OREAD);
	print("> ");
	while(line = Brdline(&in, '\n')){
		line[Blinelen(&in)-1] = 0;
		seek(fd, 0, 0);
		if(write(fd, line, Blinelen(&in)-1) < 0){
			print("!%r\n> ");
			continue;
		}
		seek(fd, 0, 0);
		while((n = read(fd, buf, sizeof(buf))) > 0){
			buf[n] = 0;
			print("%s\n", buf);
		}
		print("> ");
	}
	exits(0);
}
