/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "acd.h"

int
msfconv(Fmt *fp)
{
	Msf m;

	m = va_arg(fp->args, Msf);
	fmtprint(fp, "%d.%d.%d", m.m, m.s, m.f);
	return 0;
}

static int
playmsf(Drive *d, Msf start, Msf end)
{
	uint8_t cmd[12];

	memset(cmd, 0, sizeof cmd);
	cmd[0] = 0x47;
	cmd[3] = start.m;
	cmd[4] = start.s;
	cmd[5] = start.f;
	cmd[6] = end.m;
	cmd[7] = end.s;
	cmd[8] = end.f;

	return scsi(d->scsi, cmd, sizeof cmd, nil, 0, Snone);
}

int
playtrack(Drive *d, int start, int end)
{
	Toc *t;

	t = &d->toc;

	if(t->ntrack == 0)
		return -1;

	if(start < 0)
		start = 0;
	if(end >= t->ntrack)
		end = t->ntrack-1;
	if(end < start)
		end = start;

	return playmsf(d, t->track[start].start, t->track[end].end);
}

int
resume(Drive *d)
{
	uint8_t cmd[12];

	memset(cmd, 0, sizeof cmd);
	cmd[0] = 0x4B;
	cmd[8] = 0x01;
	return scsi(d->scsi, cmd, sizeof cmd, nil, 0, Snone);
}

int
pause(Drive *d)
{
	uint8_t cmd[12];

	memset(cmd, 0, sizeof cmd);
	cmd[0] = 0x4B;
	return scsi(d->scsi, cmd, sizeof cmd, nil, 0, Snone);
}

int
stop(Drive *d)
{
	uint8_t cmd[12];

	memset(cmd, 0, sizeof cmd);
	cmd[0] = 0x4E;
	return scsi(d->scsi, cmd, sizeof cmd, nil, 0, Snone);
}

int
eject(Drive *d)
{
	uint8_t cmd[12];

	memset(cmd, 0, sizeof cmd);
	cmd[0] = 0x1B;
	cmd[1] = 1;
	cmd[4] = 2;
	return scsi(d->scsi, cmd, sizeof cmd, nil, 0, Snone);
}

int
ingest(Drive *d)
{
	uint8_t cmd[12];

	memset(cmd, 0, sizeof cmd);
	cmd[0] = 0x1B;
	cmd[1] = 1;
	cmd[4] = 3;
	return scsi(d->scsi, cmd, sizeof cmd, nil, 0, Snone);
}

static Msf
rdmsf(uint8_t *p)
{
	Msf msf;

	msf.m = p[0];
	msf.s = p[1];
	msf.f = p[2];
	return msf;
}

static uint32_t
rdlba(uint8_t *p)
{
	return (p[0]<<16) | (p[1]<<8) | p[2];
}

/* not a Drive, so that we don't accidentally touch Drive.toc */
int
gettoc(Scsi *s, Toc *t)
{
	int i, n;
	uint8_t cmd[12];
	uint8_t resp[1024];

Again:
	memset(t, 0, sizeof(*t));
	memset(cmd, 0, sizeof cmd);
	cmd[0] = 0x43;
	cmd[1] = 0x02;
	cmd[7] = sizeof(resp)>>8;
	cmd[8] = sizeof(resp)&0xFF;

	s->changetime = 1;
	/* scsi sets nchange, changetime */
	if(scsi(s, cmd, sizeof cmd, resp, sizeof(resp), Sread) < 4)
		return -1;

	if(s->changetime == 0) {
		t->ntrack = 0;
		werrstr("no media");
		return -1;
	}

	if(t->nchange == s->nchange && t->changetime != 0)
		return 0;

	t->nchange = s->nchange;
	t->changetime = s->changetime;

	if(t->ntrack > MTRACK)
		t->ntrack = MTRACK;

DPRINT(2, "%d %d\n", resp[3], resp[2]);
	t->ntrack = resp[3]-resp[2]+1;
	t->track0 = resp[2];

	n = ((resp[0]<<8) | resp[1])+2;
	if(n < 4+8*(t->ntrack+1)) {
		werrstr("bad read0 %d %d", n, 4+8*(t->ntrack+1));
		return -1;
	}

	for(i=0; i<=t->ntrack; i++)		/* <=: track[ntrack] = end */
		t->track[i].start = rdmsf(resp+4+i*8+5);

	for(i=0; i<t->ntrack; i++)
		t->track[i].end = t->track[i+1].start;

	memset(cmd, 0, sizeof cmd);
	cmd[0] = 0x43;
	cmd[7] = sizeof(resp)>>8;
	cmd[8] = sizeof(resp)&0xFF;
	if(scsi(s, cmd, sizeof cmd, resp, sizeof(resp), Sread) < 4)
		return -1;

	if(s->changetime != t->changetime || s->nchange != t->nchange) {
		fprint(2, "disk changed underfoot; repeating\n");
		goto Again;
	}

	n = ((resp[0]<<8) | resp[1])+2;
	if(n < 4+8*(t->ntrack+1)) {
		werrstr("bad read");
		return -1;
	}

	for(i=0; i<=t->ntrack; i++)
		t->track[i].bstart = rdlba(resp+4+i*8+5);

	for(i=0; i<t->ntrack; i++)
		t->track[i].bend = t->track[i+1].bstart;

	return 0;
}

static void
dumptoc(Toc *t)
{
	int i;

	fprint(1, "%d tracks\n", t->ntrack);
	for(i=0; i<t->ntrack; i++)
		print("%d. %M-%M (%lu-%lu)\n", i+1,
			t->track[i].start, t->track[i].end,
			t->track[i].bstart, t->track[i].bend);
}

static void
ping(Drive *d)
{
	uint8_t cmd[12];

	memset(cmd, 0, sizeof cmd);
	cmd[0] = 0x43;
	scsi(d->scsi, cmd, sizeof(cmd), nil, 0, Snone);
}

static int
playstatus(Drive *d, Cdstatus *stat)
{
	uint8_t cmd[12], resp[16];

	memset(cmd, 0, sizeof cmd);
	cmd[0] = 0x42;
	cmd[1] = 0x02;
	cmd[2] = 0x40;
	cmd[3] = 0x01;
	cmd[7] = sizeof(resp)>>8;
	cmd[8] = sizeof(resp);
	if(scsi(d->scsi, cmd, sizeof(cmd), resp, sizeof(resp), Sread) < 0)
		return -1;

	switch(resp[1]){
	case 0x11:
		stat->state = Splaying;
		break;
	case 0x12:
		stat->state = Spaused;
		break;
	case 0x13:
		stat->state = Scompleted;
		break;
	case 0x14:
		stat->state = Serror;
		break;
	case 0x00:	/* not supported */
	case 0x15:	/* no current status to return */
	default:
		stat->state = Sunknown;
		break;
	}

	stat->track = resp[6];
	stat->index = resp[7];
	stat->abs = rdmsf(resp+9);
	stat->rel = rdmsf(resp+13);
	return 0;
}

void
cdstatusproc(void *v)
{
	Drive *d;
	Toc t;
	Cdstatus s;

	t.changetime = ~0;
	t.nchange = ~0;

	threadsetname("cdstatusproc");
	d = v;
	DPRINT(2, "cdstatus %d\n", getpid());
	for(;;) {
		ping(d);
	//DPRINT(2, "d %d %d t %d %d\n", d->scsi->changetime, d->scsi->nchange, t.changetime, t.nchange);
		if(playstatus(d, &s) == 0)
			send(d->cstatus, &s);
		if(d->scsi->changetime != t.changetime || d->scsi->nchange != t.nchange) {
			if(gettoc(d->scsi, &t) == 0) {
				DPRINT(2, "sendtoc...\n");
				if(debug) dumptoc(&t);
				send(d->ctocdisp, &t);
			} else
				DPRINT(2, "error: %r\n");
		}
		sleep(1000);
	}
}
