#include <u.h>
#include <libc.h>
#include <bio.h>

char	buf[4096];
Biobuf	bout;

void	id(char*, int, ulong, ulong);

void
main(int argc, char *argv[])
{
	int fd;
	char *e, *p, *q, *type;
	ulong q0, q1;

	type = 0;
	Binit(&bout, 1, OWRITE);
	ARGBEGIN{
	case 'c':
		type =  "abcdefghijklmnopqrstuvwxyz_"
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
		break;
	case 'a':
		type =  "abcdefghijklmnopqrstuvwxyz"
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ";
		break;
	case '0':
		type =  "0123456789";
		break;
	default:
		fprint(2, "usage: parse [-(ca0)]\n");
		exits("usage");
	}ARGEND
	e = getenv("helpsel");
	if(e){
		p = utfrune(e, ':');
		if(p && p>e && p[1]=='#'){
			*p++ = 0;
			q0 = strtoul(p+1, &q, 0);
			q1 = q0;
			if(q[0]==',' && q[1]=='#')
				q1 = strtoul(q+2, 0, 0);
			q1 -= q0;
			if(q1 > sizeof buf-1)
				q1 = sizeof buf-1;
			sprint(buf, "/mnt/help/%s", e);
			fd = open(buf, OREAD);
			if(fd >= 0)
				id(type, fd, q0, q1);
		}
	}
	e = getenv("helpline");
	if(e){
		p = utfrune(e, ':');
		if(p){
			q = p;
			*p++ = 0;
			if(*e)
				Bprint(&bout, "file='%s' ", e);
			while(q>e && q[-1]!='/')
				--q;
			if(*q)
				Bprint(&bout, "base='%s' ", q);
			if(q == e)
				Bprint(&bout, "dir=. ");
			else{
				while(q>e && q[-1]=='/')
					--q;
				if(q == e)
					Bprint(&bout, "dir=/ ");
				else{
					*q = 0;
					Bprint(&bout, "dir=%s ", e);
				}
			}
			Bprint(&bout, "line=%s", p);
		}
	}
	Bprint(&bout, "\n");
}

void
id(char *type, int fd, ulong p, ulong n)
{
	long q;
	char *b, *e;

	if(n>0 || type==0){
		seek(fd, p, 0);
		n = read(fd, buf, n);
		if(n > 0){
			buf[n] = 0;
			Bprint(&bout, "id='%s' ", buf);
		}
		close(fd);
		return;
	}

	/*
	 * back up a little
	 */
	q = p - 128;
	if(q < 0)
		q = 0;
	seek(fd, q, 0);

	/*
	 * now read a little and look for the identifier
	 */
	p -= q;
	n = read(fd, buf, 256);
	if(n < p)
		return;
	buf[n] = 0;
	for(b=&buf[p]; b>buf; b--)
		if(utfrune(type, b[-1]) == 0)
			break;
	for(e=&buf[p+1]; *e; e++)
		if(utfrune(type, *e) == 0)
			break;
	*e = 0;
	Bprint(&bout, "id='%s' ", b);
	close(fd);
}
