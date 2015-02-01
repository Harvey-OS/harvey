/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

void jis_in(int fd, long *notused, struct convert *out);
void jisjis_in(int fd, long *notused, struct convert *out);
void msjis_in(int fd, long *notused, struct convert *out);
void ujis_in(int fd, long *notused, struct convert *out);
void jisjis_out(Rune *base, int n, long *notused);
void ujis_out(Rune *base, int n, long *notused);
void msjis_out(Rune *base, int n, long *notused);
void big5_in(int fd, long *notused, struct convert *out);
void big5_out(Rune *base, int n, long *notused);
void gb_in(int fd, long *notused, struct convert *out);
void gb_out(Rune *base, int n, long *notused);
void gbk_in(int fd, long *notused, struct convert *out);
void gbk_out(Rune *base, int n, long *notused);
void uksc_in(int fd, long *notused, struct convert *out);
void uksc_out(Rune *base, int n, long *notused);
void html_in(int fd, long *notused, struct convert *out);
void html_out(Rune *base, int n, long *notused);
void tune_in(int fd, long *notused, struct convert *out);
void tune_out(Rune *base, int n, long *notused);

#define		emit(x)		*(*r)++ = (x)
#define		NRUNE		(Runemax+1)

extern long tab[];		/* common table indexed by Runes for reverse mappings */
