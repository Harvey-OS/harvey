
/*
 * Control interface to generic front ends.
 * written/copyrights 1997/99 by Andreas Neuhaus (and Michael Hipp)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/socket.h>

#include "mpg123.h"

#define MODE_STOPPED 0
#define MODE_PLAYING 1
#define MODE_PAUSED 2

extern struct audio_info_struct ai;
extern int buffer_pid;
extern int tabsel_123[2][3][16];

void generic_sendmsg (char *fmt, ...)
{
	va_list ap;
	printf("@");
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	printf("\n");
}

static double compute_bpf (struct frame *fr)
{
	double bpf;
	switch(fr->lay) {
		case 1:
			bpf = tabsel_123[fr->lsf][0][fr->bitrate_index];
			bpf *= 12000.0 * 4.0;
			bpf /= freqs[fr->sampling_frequency] << (fr->lsf);
			break;
		case 2:
		case 3:
			bpf = tabsel_123[fr->lsf][fr->lay-1][fr->bitrate_index];
			bpf *= 144000;
			bpf /= freqs[fr->sampling_frequency] << (fr->lsf);
			break;
		default:
			bpf = 1.0;
	}
	return bpf;
}

static double compute_tpf (struct frame *fr)
{
	static int bs[4] = { 0, 384, 1152, 1152 };
	double tpf;

	tpf = (double) bs[fr->lay];
	tpf /= freqs[fr->sampling_frequency] << (fr->lsf);
	return tpf;
}

void generic_sendstat (struct frame *fr, int no)
{
	long buffsize;
	double bpf, tpf, tim1, tim2;
	double dt = 0;
	int sno, rno;

	/* this and the 2 above functions are taken from common.c.
	/ maybe the compute_* functions shouldn't be static there
	/ so that they can also used here (performance problems?).
	/ isn't there an easier way to compute the time? */

	buffsize = xfermem_get_usedspace(buffermem);
	if (!rd || !fr)
		return;
	bpf = compute_bpf(fr);
	tpf = compute_tpf(fr);
	if (buffsize > 0 && ai.rate > 0 && ai.channels > 0) {
		dt = (double) buffsize / ai.rate / ai.channels;
		if ((ai.format & AUDIO_FORMAT_MASK) == AUDIO_FORMAT_16)
			dt *= .5;
	}
	rno = 0;
	sno = no;
	if (rd->filelen >= 0) {
		long t = rd->tell(rd);
		rno = (int)((double)(rd->filelen-t)/bpf);
		sno = (int)((double)t/bpf);
	}
	tim1 = sno * tpf - dt;
	tim2 = rno * tpf + dt;

	generic_sendmsg("F %d %d %3.2f %3.2f", sno, rno, tim1, tim2);
}

extern char *genre_table[];
extern int genre_count;
void generic_sendinfoid3 (char *buf)
{
	char info[200] = "", *c;
	int i;
	unsigned char genre;
	for (i=0, c=buf+3; i<124; i++, c++)
		info[i] = *c ? *c : ' ';
	info[i] = 0;
	genre = *c;
	generic_sendmsg("I ID3:%s%s", info, (genre<=genre_count) ? genre_table[genre] : "Unknown");
}

void generic_sendinfo (char *filename)
{
	char *s, *t;
	s = strrchr(filename, '/');
	if (!s)
		s = filename;
	else
		s++;
	t = strrchr(s, '.');
	if (t)
		*t = 0;
	generic_sendmsg("I %s", s);
}

void control_generic (struct frame *fr)
{
	struct timeval tv;
	fd_set fds;
	int n;
	int mode = MODE_STOPPED;
	int init = 0;
	int framecnt = 0;

	setlinebuf(stdout);
	printf("@R MPG123\n");
	while (1) {
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		FD_ZERO(&fds);
		FD_SET(STDIN_FILENO, &fds);

		/* play frame if no command needs to be processed */
		if (mode == MODE_PLAYING) {
			n = select(32, &fds, NULL, NULL, &tv);
			if (n == 0) {
				if (!read_frame(fr)) {
					mode = MODE_STOPPED;
					rd->close(rd);
					generic_sendmsg("P 0");
					continue;
				}
				play_frame(init,fr);
				if (init) {
					static char *modes[4] = {"Stereo", "Joint-Stereo", "Dual-Channel", "Single-Channel"};
					generic_sendmsg("S %s %d %ld %s %d %d %d %d %d %d %d %d",
						fr->mpeg25 ? "2.5" : (fr->lsf ? "2.0" : "1.0"),
						fr->lay,
						freqs[fr->sampling_frequency],
						modes[fr->mode],
						fr->mode_ext,
						fr->framesize+4,
						fr->stereo,
						fr->copyright ? 1 : 0,
						fr->error_protection ? 1 : 0,
						fr->emphasis,
						tabsel_123[fr->lsf][fr->lay-1][fr->bitrate_index],
						fr->extension);
					init = 0;
				}
				framecnt++;
				generic_sendstat(fr, framecnt);
			}
		}
		else {
			/* wait for command */
			while (1) {
				n = select(32, &fds, NULL, NULL, NULL);
				if (n > 0)
					break;
			}
		}

		/* exit on error */
		if (n < 0) {
			fprintf(stderr, "Error waiting for command: %s\n", strerror(errno));
			exit(1);
		}

		/* process command */
		if (n > 0) {
			int len;
			char buf[1024];
			char *cmd;

			/* read command */
			len = read(STDIN_FILENO, buf, sizeof(buf)-1);
			buf[len] = 0;

			/* exit on error */
			if (len < 0) {
				fprintf(stderr, "Error reading command: %s\n", strerror(errno));
				exit(1);
			}

			/* strip CR/LF at EOL */
			while (len>0 && (buf[strlen(buf)-1] == '\n' || buf[strlen(buf)-1] == '\r')) {
				buf[strlen(buf)-1] = 0;
				len--;
			}

			/* continue if no command */
			if (len == 0)
				continue;
			cmd = strtok(buf, " \t");
			if (!cmd || !strlen(cmd))
				continue;

			/* QUIT */
			if (!strcasecmp(cmd, "Q") || !strcasecmp(cmd, "QUIT"))
				break;

			/* LOAD */
			if (!strcasecmp(cmd, "L") || !strcasecmp(cmd, "LOAD")) {
				char *filename;
				filename = strtok(NULL, "");
				if (mode != MODE_STOPPED) {
					rd->close(rd);
					mode = MODE_STOPPED;
				}
				open_stream(filename, -1);
				if (rd && rd->flags & READER_ID3TAG)
					generic_sendinfoid3((char *)rd->id3buf);
				else
					generic_sendinfo(filename);
				mode = MODE_PLAYING;
				init = 1;
				framecnt = 0;
				read_frame_init();
				continue;
			}

			/* STOP */
			if (!strcasecmp(cmd, "S") || !strcasecmp(cmd, "STOP")) {
				if (mode != MODE_STOPPED) {
					rd->close(rd);
					mode = MODE_STOPPED;
					generic_sendmsg("P 0");
				}
				continue;
			}

			/* PAUSE */
			if (!strcasecmp(cmd, "P") || !strcasecmp(cmd, "PAUSE")) {
				if (mode == MODE_STOPPED)
					continue;
				if (mode == MODE_PLAYING) {
					mode = MODE_PAUSED;
					if (param.usebuffer)
						kill(buffer_pid, SIGSTOP);
					generic_sendmsg("P 1");
				} else {
					mode = MODE_PLAYING;
					if (param.usebuffer)
						kill(buffer_pid, SIGCONT);
					generic_sendmsg("P 2");
				}
				continue;
			}

			/* JUMP */
			if (!strcasecmp(cmd, "J") || !strcasecmp(cmd, "JUMP")) {
				char *spos;
				int pos, ok;

				spos = strtok(NULL, " \t");
				if (!spos)
					continue;
				if (spos[0] == '-')
					pos = framecnt + atoi(spos);
				else if (spos[0] == '+')
					pos = framecnt + atoi(spos+1);
				else
					pos = atoi(spos);

				if (mode == MODE_STOPPED)
					continue;
				ok = 1;
				if (pos < framecnt) {
					rd->rewind(rd);
					read_frame_init();
					for (framecnt=0; ok && framecnt<pos; framecnt++) {
						ok = read_frame(fr);
						if (fr->lay == 3)
							set_pointer(512);
					}
				} else {
					for (; ok && framecnt<pos; framecnt++) {
						ok = read_frame(fr);
						if (fr->lay == 3)
							set_pointer(512);
					}
				}
				continue;
			}

			/* unknown command */
			generic_sendmsg("E Unknown command '%s'", cmd);

		}
	}

	/* quit gracefully */
	if (param.usebuffer) {
		kill(buffer_pid, SIGINT);
		xfermem_done_writer(buffermem);
		waitpid(buffer_pid, NULL, 0);
		xfermem_done(buffermem);
	} else {
		audio_flush(param.outmode, &ai);
		free(pcm_sample);
	}
	if (param.outmode == DECODE_AUDIO)
		audio_close(&ai);
	if (param.outmode == DECODE_WAV)
		wav_close();
	exit(0);
}

/* EOF */
