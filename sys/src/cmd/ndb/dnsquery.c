#include <u.h>
#include <libc.h>
#include <bio.h>
#include "dns.h"
#include "ip.h"

void
main(void)
{
	int fd, n, len;
	Biobuf in;
	char *line, *p, *np;
	char buf[1024];

	fd = open("/net/dns", ORDWR);
	if(fd < 0){
		fd = open("/srv/dns", ORDWR);
		if(fd < 0){
			print("can't open /srv/dns: %r\n");
			exits(0);
		}
		if(mount(fd, "/net", MBEFORE, "") < 0){
			print("can't mount /srv/dns: %r\n");
			exits(0);
		}
		fd = open("/net/dns", ORDWR);
		if(fd < 0){
			print("can't open /net/dns: %r\n");
			exits(0);
		}
	}
	Binit(&in, 0, OREAD);
	print("> ");
	while(line = Brdline(&in, '\n')){
		n = Blinelen(&in)-1;
		line[n] = 0;

		/* inverse queries may need to be permuted */
		if(n > 4 && strcmp("ptr", &line[n-3]) == 0
		&& strstr(line, "IN-ADDR") == 0 && strstr(line, "in-addr") == 0){
			for(p = line; *p; p++)
				if(*p == ' '){
					*p = '.';
					break;
				}
			np = buf;
			len = 0;
			while(p >= line){
				len++;
				p--;
				if(*p == '.'){
					memmove(np, p+1, len);
					np += len;
					len = 0;
				}
			}
			memmove(np, p+1, len);
			np += len;
			strcpy(np, "in-addr.arpa ptr");
			line = buf;
			n = strlen(line);
		}

		seek(fd, 0, 0);
		if(write(fd, line, n) < 0){
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
