#include <alef.h>
#include <bio.h>
#include "httpd.h"

Biobuf	bin;

void
main(int argc, byte **argv)
{
	byte *meth, *vers, *uri, *search;

	(meth, vers, uri, search) = init(argc, argv);
	bin.init(0, OREAD);
	send(meth, vers, uri, search);
	exits(nil);
}

void
send(byte *meth, byte *vers, byte *uri, byte *search)
{
	int c, lastnl;

	if(vers[0] != 0){
		okheaders();
		bout.print("Date: %D\r\n", time());
		bout.print("Content-type: text/html\r\n");
		bout.print("\r\n");
	}
	bout.print("<head><title>Echo</title></head>\r\n");
	bout.print("<body><h1>Echo</h1>\r\n");
	bout.print("You requested a %s on %s", meth, uri);
	if(search)
		bout.print(" with search string %s", search);
	bout.print(".\n");

	bout.print("Your client sent the following headers:<p><pre>");
	lastnl = 1;
	while((c = bin.getc()) != Beof){
		if((c&0x7f) == '\r'){
			bout.putc(c);
			c = bin.getc();
			if(c == Beof)
				break;
		}
		bout.putc(c);
		if((c&0x7f) == '\n'){
			if(lastnl)
				break;
			lastnl = 1;
		}else
			lastnl = 0;
	}

	bout.print("</pre></body>\n");
	exits(nil);
}
