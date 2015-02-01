/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

int	mbunpack(MetaBlock *mb, uchar *p, int n);
void	mbinsert(MetaBlock *mb, int i, MetaEntry*);
void	mbdelete(MetaBlock *mb, int i, MetaEntry*);
void	mbpack(MetaBlock *mb);
uchar	*mballoc(MetaBlock *mb, int n);
void		mbinit(MetaBlock *mb, uchar *p, int n, int entries);
int mbsearch(MetaBlock*, char*, int*, MetaEntry*);
int mbresize(MetaBlock*, MetaEntry*, int);

int	meunpack(MetaEntry*, MetaBlock *mb, int i);
int	mecmp(MetaEntry*, char *s);
int	mecmpnew(MetaEntry*, char *s);

enum {
	VacDirVersion = 8,
	FossilDirVersion = 9,
};
int	vdsize(VacDir *dir, int);
int	vdunpack(VacDir *dir, MetaEntry*);
void	vdpack(VacDir *dir, MetaEntry*, int);

VacFile *_vacfileroot(VacFs *fs, VtFile *file);

int	_vacfsnextqid(VacFs *fs, uvlong *qid);
void	vacfsjumpqid(VacFs*, uvlong step);

Reprog*	glob2regexp(char*);
void	loadexcludefile(char*);
int	includefile(char*);
void	excludepattern(char*);
