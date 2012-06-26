#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <libnet.h>

/*
 *  force a connection to hangup
 */
int
hangup(int ctl)
{
	return write(ctl, "hangup", sizeof("hangup")-1) != sizeof("hangup")-1;
}
