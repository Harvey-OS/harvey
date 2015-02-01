/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

void	xmlamap(Hio *hout, AMap *v, char *tag, int indent);
void	xmlarena(Hio *hout, Arena *v, char *tag, int indent);
void	xmlindex(Hio *hout, Index *v, char *tag, int indent);

void	xmlaname(Hio *hout, char *v, char *tag);
void	xmlscore(Hio *hout, u8int *v, char *tag);
void	xmlsealed(Hio *hout, int v, char *tag);
void	xmlu32int(Hio *hout, u32int v, char *tag);
void	xmlu64int(Hio *hout, u64int v, char *tag);

void	xmlindent(Hio *hout, int indent);
