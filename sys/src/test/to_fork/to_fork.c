#include <u.h>
#include <libc.h>
#include <stdio.h>

void
main()
{
	int pid = fork();
	if(pid == 0) {
		printf("I'm the father\n");
	} else if(pid > 0) {
		printf("I'm the child\n");
	} else {
		printf("fork() error\n");
	}
}
