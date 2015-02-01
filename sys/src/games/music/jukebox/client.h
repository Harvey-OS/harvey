/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

char*	getroot(void);
void	doplay(char*);
void	fillbrowsebot(char*);
void	fillbrowsetop(char*);
void	fillplaytext(char*);
void	addchild(char*, char*);
void	addparent(char*);
char	*getoneliner(char*);
char	*getparent(char*);
void	addplaytext(char*);
