#include <u.h>
#include <libc.h>
#include <libg.h>
#include <frame.h>
#include <regexp.h>
#include "dat.h"
#include "fns.h"

void	xtrnctl(void);

ulong
lineno(Rune *s, ulong q0)
{
	ulong n;

	n = 1;
	if(s) while(*s && q0-->0)
		if(*s++ == '\n')
			n++;
	return n;
}

void
spawn(int slot, char *prog, char *argv[])
{
	int fd, n;
	char *p;
	Rune *r;
	ulong q0, c0, c1;

	sprint((char*)genbuf, "%d", slot);
	switch(rfork(RFENVG|RFNAMEG|RFNOTEG|RFFDG|RFPROC|RFNOWAIT)){
	case 0:
		if(mount(cfd, "/mnt/help", 0, (char*)genbuf, "") < 0)
			error("client mount");
		if(bind("/mnt/help", "/dev", MBEFORE) < 0)
			error("client bind");
		/* tell him where selection is */
		fd = create("/env/helpsel", 1, 0664);
		if(fd >= 0){
			if(curt){
				p = (char*)genbuf;
				p += sprint(p, "%lud/", curt->page->id);
				if(curt == &curt->page->tag){
					strcpy(p, "tag:");
					p += 4;
				}else{
					strcpy(p, "body:");
					p += 5;
				}
				/* convert from runes to chars in offset */
				c0 = 0;
				r = curt->s;
				for(q0=0; q0<curt->q0; q0++)
					c0 += runelen(*r++);
				c1 = c0;
				for(; q0<curt->q1; q0++)
					c1 += runelen(*r++);
				if(c1 > c0)
					sprint(p, "#%lud,#%lud\n",
						c0, c1);
				else
					sprint(p, "#%lud\n", c0);
				write(fd, genbuf, strlen((char*)genbuf));
			}
			close(fd);
		}
		fd = create("/env/helpline", 1, 0664);
		if(fd >= 0){
			if(curt){
				r = curt->page->tag.s;
				n = 0;
				while(*r && *r!='\t' && *r!=' ')
					n++, r++;
				n = sprint((char*)genbuf, "%.*S:%lud\n",
					n, curt->page->tag.s,
					lineno(curt->page->body.s, curt->page->body.q0));
				write(fd, genbuf, n);
			}
			close(fd);
		}
		close(sfd);
		close(cfd);
		close(clockfd);
		close(timefd);
		bclose();
		close(0);
		close(1);
		if(open("/dev/null", OREAD) != 0)
			error("/dev/null 0 client open");
		if(open("/dev/cons", OWRITE) != 1)
			error("/dev/cons 1 client open");
		dup(1, 2);
		exec(prog, argv);
		exits("exec error");

	case -1:
		error("fork");
	}
}

void
xtrnindex(Page *p, String *index)
{
	Rune *r;
	char buf[32];

	while(p){
		sprint(buf, "%lud\t", p->id);
		r = tmprstr(buf);
		Strinsert(index, r, rstrlen(r), index->n);
		free(r);
		Strinsert(index, p->tag.s, p->tag.n, index->n);
		Strinsert(index, L"\n", 1, index->n);
		xtrnindex(p->down, index);
		p = p->next;
	}
}

void
xtrnfork(char *prog, int argc, char *argv[], Page *page, Text *curt)
{
	int i, n;
	Client *c;

	USED(curt, page, argc);
	/* slot 0 is for slaves */
	for(i=1; client[i].busy; i++)
		if(i == NSLOT-1)
			return;
	c = &client[i];
	memset(c, 0, sizeof(Client));
	c->busy = 1;
	c->slot = i;
	c->wcnt = -1;
	c->index = emalloc(sizeof(String));
	memset(c->index, 0, sizeof(String));
	xtrnindex(rootpage, c->index);
	n = strlen(argv[0])+1;
	c->argv0 = emalloc(n);
	memmove(c->argv0, argv[0], n);
	spawn(c->slot, prog, argv);
	c->p = newproc(xtrnctl, c);
}

void
xtrn(int argc, char *argv[], Page *page, Text *curt)
{
	char *prog, *file;
	Rune *d1;
	int n;

	prog = argv[0];
	file = prog;
	if(prog[0]!='/' && strncmp(prog, "./", 2)!=0){
		d1 = endslash(page->tag.s);
		if(d1){
			n = d1-page->tag.s;
			file = emalloc(UTFmax*n+strlen(argv[0])+1);
			sprint(file, "%.*S%s", n, page->tag.s, argv[0]);
		}
	}
	if(access(file, 001) < 0){
		if(prog[0]=='/' || strncmp(prog, "./", 2)==0)
			goto Return;
		if(prog != argv[0])
			free(prog);
		prog = emalloc(5+strlen(argv[0])+1);
		sprint(prog, "/bin/%s", argv[0]);
		if(access(prog, 001) < 0){
			free(prog);
			return;
		}
	}else
		prog = file;
	puttag(rootpage, argv[0], 0);
	bflush();
	xtrnfork(prog, argc, argv, page, curt);
    Return:
	if(prog != argv[0])
		free(prog);
}

void
xtrnwrite(Client *c, int fid, uchar *buf, int cnt)
{
	c->wbuf = buf;
	c->wfid = fid;
	c->wcnt = cnt;
	c->wbuf[cnt] = 0;	/* there's room */
	run(c->p);
}

void
xtrndelete(Client *c)
{
	c->close = 1;
	run(c->p);
}

void
xtrnctl(void)
{
	Client *c;

	c = proc.p->arg;
	for(;;){
		if(c->wcnt >= 0){
			errs(c->wbuf);
			c->wcnt = -1;
		}
		if(c->close){
			unputtag(rootpage, c->argv0);
			free(c->argv0);
			if(c->index->s)
				free(c->index->s);
			free(c->index);
			c->busy = 0;
			c->p->dead = 1;
		}
		sched();
	}
}
