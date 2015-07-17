#include <u.h>
#include <libc.h>
#include "usb.h"

void
main(int argc, char *argv[])
{
	Usbconfig *confs;
	int nconfs;
	int fd, epdata, epctl;
	int i;

	confs = nil;
	epdata = -1;
	epctl = -1;
	fd = open(argv[1], ORDWR);
	nconfs = usbconfread(fd, &confs);
	for(i = 0; i < nconfs; i++){
		if(usbconfprint(1, confs+i) == -1){
			fprint(2, "usbprint fail!\n");
			goto fail;
		}
	}
	epdata = usbopen(fd, confs, 0, &epctl);
	if(epdata == -1){
		fprint(2, "usbopen fail!\n");
		goto fail;
	}

	for(i = 0; i < 128; i++){
		char buf[8];
		int j, n;
		n = read(epdata, buf, sizeof buf);
		for(j = 0; j < n; j++)
			print("0x%02x%s", buf[j], j == n-1 ? "\n" : ", ");
	}

fail:
	if(confs != nil)
		free(confs);
	close(fd);
	if(epdata != -1)
		close(epdata);
	if(epctl != -1)
		close(epctl);
	print("sizeof(confs[0]) = %d\n", sizeof confs[0]);
	exits(nil);
}
