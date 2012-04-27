#include <u.h>
#include <libc.h>
#include <ip.h>
#include <thread.h>
#include <sunrpc.h>

typedef struct SunMsgUdp SunMsgUdp;
struct SunMsgUdp
{
	SunMsg	msg;
	Udphdr	udp;
};

typedef struct Arg Arg;
struct Arg
{
	SunSrv	*srv;
	Channel	*creply;
	Channel	*csync;
	int 	fd;
};

enum
{
	UdpMaxRead = 65536+Udphdrsize
};

static void
sunUdpRead(void *v)
{
	int n;
	uchar *buf;
	Arg arg = *(Arg*)v;
	SunMsgUdp *msg;

	sendp(arg.csync, 0);

	buf = emalloc(UdpMaxRead);
	while((n = read(arg.fd, buf, UdpMaxRead)) > 0){
		if(arg.srv->chatty)
			fprint(2, "udp got %d (%d)\n", n, Udphdrsize);
		msg = emalloc(sizeof(SunMsgUdp));
		memmove(&msg->udp, buf, Udphdrsize);
		msg->msg.data = emalloc(n);
		msg->msg.count = n-Udphdrsize;
		memmove(msg->msg.data, buf+Udphdrsize, n-Udphdrsize);
		memmove(&msg->udp, buf, Udphdrsize);
		msg->msg.creply = arg.creply;
		if(arg.srv->chatty)
			fprint(2, "message %p count %d\n", msg, msg->msg.count);
		sendp(arg.srv->crequest, msg);
	}
}

static void
sunUdpWrite(void *v)
{
	uchar *buf;
	Arg arg = *(Arg*)v;
	SunMsgUdp *msg;

	sendp(arg.csync, 0);

	buf = emalloc(UdpMaxRead);
	while((msg = recvp(arg.creply)) != nil){
		memmove(buf+Udphdrsize, msg->msg.data, msg->msg.count);
		memmove(buf, &msg->udp, Udphdrsize);
		msg->msg.count += Udphdrsize;
		if(write(arg.fd, buf, msg->msg.count) != msg->msg.count)
			fprint(2, "udpWrite: %r\n");
		free(msg->msg.data);
		free(msg);
	}
}

int
sunSrvUdp(SunSrv *srv, char *address)
{
	int acfd, fd;
	char adir[40], data[60];
	Arg *arg;

	acfd = announce(address, adir);
	if(acfd < 0)
		return -1;
	if(write(acfd, "headers", 7) < 0){
		werrstr("setting headers: %r");
		close(acfd);
		return -1;
	}
	snprint(data, sizeof data, "%s/data", adir);
	if((fd = open(data, ORDWR)) < 0){
		werrstr("open %s: %r", data);
		close(acfd);
		return -1;
	}
	close(acfd); 
	
	arg = emalloc(sizeof(Arg));
	arg->fd = fd;
	arg->srv = srv;
	arg->creply = chancreate(sizeof(SunMsg*), 10);
	arg->csync =  chancreate(sizeof(void*), 10);

	proccreate(sunUdpRead, arg, SunStackSize);
	proccreate(sunUdpWrite, arg, SunStackSize);
	recvp(arg->csync);
	recvp(arg->csync);
	chanfree(arg->csync);
	free(arg);

	return 0;
}
